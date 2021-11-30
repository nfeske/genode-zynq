/*
 * \brief  Driver for the axi_dmac IP core from Analog Devices
 * \author Johannes Schlatow
 * \date   2021-10-05
 *
 * See https://wiki.analog.com/resources/fpga/docs/axi_dmac
 *
 * Current limitations:
 * - 2D-transfer support not implemented
 * - cyclic transfer not implemented
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__DRIVERS__DMAC_H_
#define _INCLUDE__DRIVERS__DMAC_H_

#include <util/bit_array.h>
#include <platform_session/device.h>

namespace Ad {
	using namespace Genode;

	class Axi_dmac_base;

	template<bool READ_SUPPORT, bool WRITE_SUPPORT>
	class Axi_dmac;

	typedef Axi_dmac<true,false> Axi_dmac_rx;
	typedef Axi_dmac<false,true> Axi_dmac_tx;
}


class Ad::Axi_dmac_base : public Platform::Device::Mmio
{
	public:
		struct Read_not_supported  : Exception { };
		struct Write_not_supported : Exception { };
		struct Device_error        : Exception { };
		struct Buffer_exceeded     : Exception { };

	protected:

		/**
		 * Registers
		 */
		struct Version : Register<0x000, 32>
		{
			struct Patch : Bitfield< 0,8>  { };
			struct Minor : Bitfield< 8,8>  { };
			struct Major : Bitfield<16,16> { };
		};

		struct Peripheral_id : Register<0x004, 32>
		{
			struct Value : Bitfield<0,32> { };
		};

		struct Identification : Register<0x00c, 32>
		{
			struct Value : Bitfield<0,32> {
				enum {
					DMAC = 0x444D4143
				};
			};
		};

		/* only valid from version >= 4.3 */
		struct Interface : Register<0x010, 32>
		{
			enum Dma_type {
				MEMORY_MAP = 0,
				STREAM     = 1,
				FIFO       = 2
			};

			struct Bytes_per_beat_dst_log2 : Bitfield< 0,4> { };
			struct Dma_type_dst            : Bitfield< 4,2> { };
			struct Bytes_per_beat_src_log2 : Bitfield< 8,4> { };
			struct Dma_type_src            : Bitfield<12,2> { };
		};

		struct Irq_mask : Register<0x080, 32>
		{
			struct Transfer_queued    : Bitfield<0,1> { };
			struct Transfer_completed : Bitfield<1,1> { };
		};

		struct Irq_status : Register<0x084, 32>
		{
			struct Transfer_queued    : Bitfield<0,1> { };
			struct Transfer_completed : Bitfield<1,1> { };
		};

		struct Ctrl : Register<0x400, 32>
		{
			struct Enable : Bitfield<0,1> { };
			struct Pause  : Bitfield<1,1> { };
		};

		struct Transfer_id : Register<0x404, 32>
		{
			enum {
				MAX_TRANSFER_ID = 30
			};
			struct Next : Bitfield<0,2> { };
		};

		struct Transfer_submit : Register<0x408, 32>
		{
			struct Queue : Bitfield<0,1> { };
		};

		struct Transfer_progress : Register<0x448, 32>
		{
			struct Bytes : Bitfield<0,24> { };
		};

		struct Flags : Register<0x40c, 32>
		{
			struct Cyclic            : Bitfield<0,1> { };
			struct Tlast             : Bitfield<1,1> { };
			struct Partial_reporting : Bitfield<2,1> { };
		};

		struct Transfer_dst : Register<0x410, 32>
		{
			struct Address : Bitfield<0,32> { };
		};

		struct Transfer_src : Register<0x414, 32>
		{
			struct Address : Bitfield<0,32> { };
		};

		struct Transfer_len : Register<0x418, 32>
		{
			struct Bytes : Bitfield<0,32> { };
		};

		struct Transfer_done : Register_array<0x428, 32, 31, 1>
		{ };

		/**
		 * Helper for acquiring a DMA buffer from a platform_session and attaching
		 * it to the local address space.
		 */
		class Dma_buffer
		{
			private:

				/**
				 * Noncopyable
				 */
				Dma_buffer(Dma_buffer const &);
				Dma_buffer &operator = (Dma_buffer const &);

			public:
				size_t const               capacity;
				size_t                     size     { 0 };

				Platform::Connection      &platform;

				Ram_dataspace_capability   ds_cap   { platform.alloc_dma_buffer(capacity, UNCACHED) };
				addr_t const               dma_addr { platform.dma_addr(ds_cap) };
				Genode::Attached_dataspace ds;
				void * const               ptr      { (void*)ds.local_addr<addr_t>() };

				bool                       used     { false };

				Dma_buffer(size_t max_size, Env &env, Platform::Connection &platform)
				: capacity(max_size),
				  platform(platform),
				  ds(env.rm(), ds_cap)
				{ }

				~Dma_buffer()
				{ platform.free_dma_buffer(ds_cap); }
		};

		/**
		 * Member
		 */
		Dma_buffer            _buffers[Transfer_id::MAX_TRANSFER_ID+1];
		bool                  _read_support  { true };
		bool                  _write_support { true };

		Axi_dmac_base(Genode::Env          &env,
		              Platform::Connection &platform,
		              Platform::Device     &device,
		              size_t max_transfer_len)
		: Platform::Device::Mmio(device),
		  _buffers{{ max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform },
		           { max_transfer_len, env, platform }}
		{
			log("Initialising AXI DMAC device at ", Hex((unsigned)local_addr<unsigned>()));

			if (read<Identification::Value>() != Identification::Value::DMAC) {
				error("AXI DMAC identification failed");
				throw Device_error();
			}

			log("Found AXI DMAC with peripheral id ", Hex(read<Peripheral_id::Value>()),
			    " and version ", read<Version::Major>(),
			    ".",             read<Version::Minor>(),
			    ".",             read<Version::Patch>());

			if (read<Version::Major>() == 4 && read<Version::Minor>() <= 3)
				warning("Version does not implement interface register");
			else {
				log("Destination type: ", read<Interface::Dma_type_dst>());
				log("Source type:      ", read<Interface::Dma_type_src>());

				_read_support  = read<Interface::Dma_type_dst>() == Interface::MEMORY_MAP;
				_write_support = read<Interface::Dma_type_src>() == Interface::MEMORY_MAP;
			}
		}

	public:

		void enable()  { write<Ctrl::Enable>(1); }
		void disable() { write<Ctrl::Enable>(0); }
		void pause()   { write<Ctrl::Pause>(1); }
		void resume()  { write<Ctrl::Pause>(0); }
};


template<bool READ_SUPPORT, bool WRITE_SUPPORT>
class Ad::Axi_dmac : public Ad::Axi_dmac_base
{
	public:
		enum Result {
			OKAY,
			DEVICE_ERROR,
			NOT_SUPPORTED,
			BUFFER_EXCEEDED,
			QUEUE_FULL
		};

	private:

		using Regs = Axi_dmac_base;

		/**
		 * Member
		 */
		Platform::Device::Irq _irq;
		unsigned              _next_recv_transfer { 0 };

		Dma_buffer & _enqueue_read_transfer(size_t bytes)
		{
			/* determine next transfer ID */
			unsigned const next_id = read<Regs::Transfer_id::Next>();

			if (next_id > Regs::Transfer_id::MAX_TRANSFER_ID)
				throw Device_error();

			/* get corresponding DMA buffer */
			Dma_buffer &buf = _buffers[next_id];

			if (bytes > buf.capacity)
				throw Buffer_exceeded();

			if (!buf.used) {
				buf.size = bytes;

				write<Regs::Transfer_len::Bytes>(buf.size-1);
				write<Regs::Transfer_dst::Address>(buf.dma_addr);

				/* submit transfer */
				write<Regs::Transfer_submit::Queue>(1);
			}

			return buf;
		}

		/**
		 * Fill queue with read transfers
		 */
		void _fill_read_transfers(size_t bytes)
		{
			try {
				while (true) {
					Dma_buffer &buf = _enqueue_read_transfer(bytes);
					if (buf.used)
						break;
					
					buf.used = true;
				}
			} catch (Buffer_exceeded) {
				error("Buffer exceeded during _enqueue_read_transfer()");
			} catch (Device_error) {
				error("Device error during _enqueue_read_transfer()"); }
		}

	public:

		Axi_dmac(Env &env,
		         Platform::Connection &platform,
		         Platform::Device &device,
		         size_t max_transfer_len)
		: Axi_dmac_base(env, platform, device, max_transfer_len),
		 _irq(device)
		{
			if (READ_SUPPORT && !_read_support)
				throw Read_not_supported();
			if (WRITE_SUPPORT && !_write_support)
				throw Write_not_supported();

			write<Regs::Flags::Cyclic>(0);
			enable();
		}

		/**
		 * Place a write transfer into the DMA queue.
		 *
		 * \param write_to_buf     func(void*,size_t) called for filling the DMA buffer
		 * \param blocking         only return when the transfer has been queued
		 */
		template <typename FUNC>
		Result write_transfer(FUNC && write_to_buf, bool blocking=false)
		{
			if (!write_support())
				return NOT_SUPPORTED;

			/* check whether a queuing operation is still pending */
			if (read<Regs::Transfer_submit::Queue>())
				return QUEUE_FULL;

			/* determine next transfer ID */
			unsigned const next_id = read<Regs::Transfer_id::Next>();

			if (next_id > Regs::Transfer_id::MAX_TRANSFER_ID)
				return DEVICE_ERROR;

			/* get corresponding DMA buffer */
			Dma_buffer &buf = _buffers[next_id];

			/* check whether transfer is still ongoing */
			buf.used = buf.used ? !(read<Regs::Transfer_done>(next_id)) : false;

			if (buf.used)
				return QUEUE_FULL;

			/* call write_to_buf */
			buf.size = write_to_buf(buf.ptr, buf.capacity);

			if (buf.size > buf.capacity)
				return BUFFER_EXCEEDED;

			buf.used = true;

			/* set transfer length and address */
			write<Regs::Transfer_len::Bytes>(buf.size-1);
			write<Regs::Transfer_src::Address>(buf.dma_addr);

			/* submit transfer */
			write<Regs::Transfer_submit::Queue>(1);

			/* wait until queued */
			while (blocking && read<Regs::Transfer_submit::Queue>());

			return OKAY;
		}

		/**
		 * Perform a blocking read transfer.
		 *
		 * \param bytes            maximum number of bytes to read
		 * \param read_from_buf    func(void*,size_t) called for reading from the DMA buffer
		 */
		template <typename FUNC>
		Result read_single_transfer(size_t bytes, FUNC && read_from_buf)
		{
			if (!read_support())
				return NOT_SUPPORTED;

			Dma_buffer &buf = _enqueue_read_transfer(bytes);

			/* sanity check if called when with irq handling enabled */
			if (buf.used)
				return QUEUE_FULL;

			/* wait until queued */
			while (read<Regs::Transfer_submit::Queue>());

			/* call user handler */
			read_from_buf(buf.ptr, buf.size);
		}

		/**
		 * Read completed transfers
		 *
		 * \param read_from_buf    func(void*,size_t) called for reading from the DMA buffer
		 */
		template <typename FUNC>
		void read_completed_transfers(FUNC && read_from_buf)
		{
			/**
			 * Check which transfers are done.
			 *
			 * Transfers are enqueued in order of their ids. The ids start
			 * at 0 once MAX_TRANSFER_ID was reached (at the latest).
			 * We must therefore continue checking for completed transfers where
			 * we left off during the last call of handle_irq().
			 *
			 * We must also make sure that the done state does not change during
			 * iteration. For convenient access to individual bits in the the
			 * samples register value, we interpret the latter as a Bit_array_base.
			 */
			unsigned       last_received { 0 };
			long unsigned  raw_value     { read<Regs::Transfer_done>() };
			Bit_array_base done          { 32, &raw_value };

			for (size_t i = 0; i <= Regs::Transfer_id::MAX_TRANSFER_ID; ++i) {
				size_t id = (i + _next_recv_transfer) % (Regs::Transfer_id::MAX_TRANSFER_ID + 1);

				if (done.get(id, 1))
					if (Dma_buffer &buf = _buffers[id]; buf.used) {
						/* call user handler */
						read_from_buf(buf.ptr, buf.size);

						buf.used = false;
						last_received = id;
					}
			}

			/* remember where to start from when handle_irq() is called again */
			_next_recv_transfer = last_received + 1;

			/**
			 * refill queue with read transfers re-using the initially set
			 * size of the first buffer
			 */
			_fill_read_transfers(_buffers[0].size);
		}

		/**
		 * Handle transfer complete irqs from transfers.
		 */
		template <typename FUNC>
		void handle_irq(FUNC && fn)
		{
			if (!read<Regs::Irq_status::Transfer_completed>())
				return;

			/* clear irq status */
			write<Regs::Irq_status::Transfer_completed>(1);

			fn();
		}

		void irq_sigh(Signal_context_capability cap) {
			_irq.sigh(cap); }
		
		void irq_ack() { _irq.ack(); }

		/**
		 * Enable rx by placing read transfers into the queue.
		 */
		void enable_rx(size_t transfer_bytes)
		{
			if (!read_support())
				throw Read_not_supported();

			enable_irq();

			_fill_read_transfers(transfer_bytes);
		}

		/**
		 * Enable transfer complete interrupts
		 */
		void enable_irq()
		{
			/* clear status */
			write<Regs::Irq_status>(Regs::Irq_status::Transfer_completed::bits(1) |
			                        Regs::Irq_status::Transfer_queued::bits(1));

			/* unmask transfer complete */
			write<Regs::Irq_mask::Transfer_completed>(0);
		}

		constexpr bool read_support()  const { return READ_SUPPORT; }
		constexpr bool write_support() const { return WRITE_SUPPORT; }
};

#endif /* _INCLUDE__DRIVERS__DMAC_H_ */

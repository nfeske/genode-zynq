/*
 * \brief   Zynq zedboard specific board definitions
 * \author  Johannes Schlatow
 * \date    2021-08-18
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _SRC__INCLUDE__HW__SPEC__ARM__ZYNQ_ZEDBOARD_BOARD_H_
#define _SRC__INCLUDE__HW__SPEC__ARM__ZYNQ_ZEDBOARD_BOARD_H_

#include <hw/spec/arm/zynq_zedboard.h>
#include <drivers/uart/xilinx.h>
#include <hw/spec/arm/boot_info.h>

namespace Hw::Zynq_zedboard_board {

	using namespace Zynq_zedboard;
	using Serial   = Genode::Xilinx_uart;
}

#endif /* _SRC__INCLUDE__HW__SPEC__ARM__ZYNQ_ZEDBOARD_BOARD_H_ */

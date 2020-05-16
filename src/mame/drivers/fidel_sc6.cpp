// license:BSD-3-Clause
// copyright-holders:hap
// thanks-to:yoyo_chessboard, Berger
/******************************************************************************

Fidelity Sensory Chess Challenger 6 (model SC6)
Fidelity Mini Sensory Chess Challenger (model MSC, 1982 version)

SC6 Hardware notes:
- PCB label 510-1045B01
- INS8040N-11 MCU, 11MHz XTAL
- external 4KB ROM 2332 101-1035A01, in module slot
- buzzer, 2 7seg LEDs, 8*8 chessboard buttons

SC6 released modules, * denotes not dumped yet:
- *BO6: Book Openings I
- *CG6: Greatest Chess Games 1
- SC6: pack-in, original program

SC6 program is contained in BO6 and CG6.

-------------------------------------------------------------------------------

MSC hardware notes:
- PCB label 510-1044B01
- P8049H MCU, 11MHz XTAL
- 2KB internal ROM, module slot
- buzzer, 18 leds, 8*8 chessboard buttons

I/O is identical to SC6.

MSC released modules, * denotes not dumped yet:
- CAC: Challenger Advanced Chess
- *CBO: Challenger Book Openings
- *CGG: Challenger Greatest Games

The modules take over the internal ROM, by asserting the EA pin.

2 MSC versions exist, they can be distinguished from the button panel design.
The 2nd version has rectangular buttons. The one in MAME came from the 2nd one.

TODO:
- MSC MCU is currently emulated as I8039, due to missing EA pin emulation
- msc internal artwork (game works fine, but not really playable at the moment)

******************************************************************************/

#include "emu.h"
#include "cpu/mcs48/mcs48.h"
#include "machine/sensorboard.h"
#include "sound/dac.h"
#include "sound/volt_reg.h"
#include "video/pwm.h"
#include "bus/generic/slot.h"
#include "bus/generic/carts.h"

#include "softlist.h"
#include "speaker.h"

// internal artwork
#include "fidel_sc6.lh" // clickable


namespace {

class sc6_state : public driver_device
{
public:
	sc6_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_rom(*this, "maincpu"),
		m_board(*this, "board"),
		m_display(*this, "display"),
		m_dac(*this, "dac"),
		m_cart(*this, "cartslot"),
		m_inputs(*this, "IN.0")
	{ }

	// machine configs
	void msc(machine_config &config);
	void sc6(machine_config &config);

protected:
	virtual void machine_start() override;

private:
	// devices/pointers
	required_device<mcs48_cpu_device> m_maincpu;
	required_region_ptr<u8> m_rom;
	required_device<sensorboard_device> m_board;
	required_device<pwm_display_device> m_display;
	required_device<dac_bit_interface> m_dac;
	required_device<generic_slot_device> m_cart;
	required_ioport m_inputs;

	// address maps
	void msc_map(address_map &map);
	void sc6_map(address_map &map);

	// I/O handlers
	void update_display();
	void mux_w(u8 data);
	void select_w(u8 data);
	u8 rom_r(offs_t offset);

	u8 read_inputs();
	u8 input_r();
	DECLARE_READ_LINE_MEMBER(input6_r);
	DECLARE_READ_LINE_MEMBER(input7_r);

	u8 m_led_select = 0;
	u8 m_inp_mux = 0;
};

void sc6_state::machine_start()
{
	// register for savestates
	save_item(NAME(m_led_select));
	save_item(NAME(m_inp_mux));
}



/******************************************************************************
    I/O
******************************************************************************/

// MCU ports/generic

void sc6_state::update_display()
{
	// 2 7seg leds
	m_display->matrix(m_led_select, 1 << m_inp_mux);
}

void sc6_state::mux_w(u8 data)
{
	// P24-P27: 7442 A-D (or 74145)
	// 7442 0-8: input mux, led data
	m_inp_mux = data >> 4 & 0xf;
	update_display();

	// 7442 9: speaker out
	m_dac->write(BIT(1 << m_inp_mux, 9));
}

void sc6_state::select_w(u8 data)
{
	// P16,P17: led select
	m_led_select = ~data >> 6 & 3;
	update_display();
}

u8 sc6_state::rom_r(offs_t offset)
{
	// MSC reads from cartridge if it's inserted(A12 high), otherwise from internal ROM
	return m_cart->exists() ? m_cart->read_rom(offset | 0x1000) : m_rom[offset & 0x7ff];
}

u8 sc6_state::read_inputs()
{
	u8 data = 0;

	// read chessboard sensors
	if (m_inp_mux < 8)
		data = m_board->read_file(m_inp_mux);

	// read button panel
	else if (m_inp_mux == 8)
		data = m_inputs->read();

	return ~data;
}

u8 sc6_state::input_r()
{
	// P10-P15: multiplexed inputs low
	return (read_inputs() & 0x3f) | 0xc0;
}

READ_LINE_MEMBER(sc6_state::input6_r)
{
	// T0: multiplexed inputs bit 6
	return read_inputs() >> 6 & 1;
}

READ_LINE_MEMBER(sc6_state::input7_r)
{
	// T1: multiplexed inputs bit 7
	return read_inputs() >> 7 & 1;
}



/******************************************************************************
    Address Maps
******************************************************************************/

void sc6_state::msc_map(address_map &map)
{
	map(0x0000, 0x0fff).r(FUNC(sc6_state::rom_r));
}

void sc6_state::sc6_map(address_map &map)
{
	map(0x0000, 0x0fff).r("cartslot", FUNC(generic_slot_device::read_rom));
}



/******************************************************************************
    Input Ports
******************************************************************************/

static INPUT_PORTS_START( sc6 )
	PORT_START("IN.0")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_1) PORT_CODE(KEYCODE_1_PAD) PORT_NAME("RV / Pawn")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_2) PORT_CODE(KEYCODE_2_PAD) PORT_NAME("DM / Knight")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_3) PORT_CODE(KEYCODE_3_PAD) PORT_NAME("TB / Bishop")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_4) PORT_CODE(KEYCODE_4_PAD) PORT_NAME("LV / Rook")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_5) PORT_CODE(KEYCODE_5_PAD) PORT_NAME("PV / Queen")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_6) PORT_CODE(KEYCODE_6_PAD) PORT_NAME("PB / King")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_DEL) PORT_CODE(KEYCODE_BACKSPACE) PORT_NAME("CL")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_R) PORT_NAME("RE")
INPUT_PORTS_END

static INPUT_PORTS_START( msc )
	PORT_INCLUDE( sc6 )

	PORT_MODIFY("IN.0")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_3) PORT_CODE(KEYCODE_3_PAD) PORT_NAME("Speaker / Bishop")
INPUT_PORTS_END



/******************************************************************************
    Machine Configs
******************************************************************************/

void sc6_state::msc(machine_config &config)
{
	/* basic machine hardware */
	I8039(config, m_maincpu, 11_MHz_XTAL); // actually I8049
	m_maincpu->set_addrmap(AS_PROGRAM, &sc6_state::msc_map);
	m_maincpu->p2_out_cb().set(FUNC(sc6_state::mux_w));
	m_maincpu->p1_in_cb().set(FUNC(sc6_state::input_r));
	m_maincpu->p1_out_cb().set(FUNC(sc6_state::select_w));
	m_maincpu->t0_in_cb().set(FUNC(sc6_state::input6_r));
	m_maincpu->t1_in_cb().set(FUNC(sc6_state::input7_r));

	SENSORBOARD(config, m_board).set_type(sensorboard_device::BUTTONS);
	m_board->init_cb().set(m_board, FUNC(sensorboard_device::preset_chess));
	m_board->set_delay(attotime::from_msec(150));

	/* video hardware */
	PWM_DISPLAY(config, m_display).set_size(2, 9);
	m_display->set_segmask(0x3, 0xff);
	config.set_default_layout(layout_fidel_sc6);

	/* sound hardware */
	SPEAKER(config, "speaker").front_center();
	DAC_1BIT(config, m_dac).add_route(ALL_OUTPUTS, "speaker", 0.25);
	VOLTAGE_REGULATOR(config, "vref").add_route(0, "dac", 1.0, DAC_VREF_POS_INPUT);

	/* cartridge */
	GENERIC_CARTSLOT(config, "cartslot", generic_plain_slot, "fidel_msc");
	SOFTWARE_LIST(config, "cart_list").set_original("fidel_msc");
}

void sc6_state::sc6(machine_config &config)
{
	msc(config);

	/* basic machine hardware */
	I8040(config.replace(), m_maincpu, 11_MHz_XTAL);
	m_maincpu->set_addrmap(AS_PROGRAM, &sc6_state::sc6_map);
	m_maincpu->p2_out_cb().set(FUNC(sc6_state::mux_w));
	m_maincpu->p1_in_cb().set(FUNC(sc6_state::input_r));
	m_maincpu->p1_out_cb().set(FUNC(sc6_state::select_w));
	m_maincpu->t0_in_cb().set(FUNC(sc6_state::input6_r));
	m_maincpu->t1_in_cb().set(FUNC(sc6_state::input7_r));

	/* video hardware */
	m_display->set_size(2, 7);
	m_display->set_segmask(0x3, 0x7f);
	config.set_default_layout(layout_fidel_sc6);

	/* cartridge */
	GENERIC_CARTSLOT(config.replace(), "cartslot", generic_plain_slot, "fidel_sc6").set_must_be_loaded(true);
	SOFTWARE_LIST(config.replace(), "cart_list").set_original("fidel_sc6");
}



/******************************************************************************
    ROM Definitions
******************************************************************************/

ROM_START( fscc6 )
	ROM_REGION( 0x1000, "maincpu", ROMREGION_ERASE00 )
	// none here, it's in the module slot
ROM_END

ROM_START( miniscc )
	ROM_REGION( 0x0800, "maincpu", 0 )
	ROM_LOAD("100-1012b01", 0x0000, 0x0800, CRC(ea3261f7) SHA1(1601358fdf0eee0b973c0f4c78bf679b8dada72a) ) // internal ROM
ROM_END

} // anonymous namespace



/******************************************************************************
    Drivers
******************************************************************************/

//    YEAR  NAME     PARENT  CMP MACHINE  INPUT  CLASS      INIT        COMPANY, FULLNAME, FLAGS
CONS( 1982, fscc6,   0,       0, sc6,     sc6,   sc6_state, empty_init, "Fidelity Electronics", "Sensory Chess Challenger \"6\"", MACHINE_SUPPORTS_SAVE | MACHINE_CLICKABLE_ARTWORK )
CONS( 1982, miniscc, 0,       0, msc,     msc,   sc6_state, empty_init, "Fidelity Electronics", "Mini Sensory Chess Challenger (1982 version)", MACHINE_SUPPORTS_SAVE | MACHINE_CLICKABLE_ARTWORK )

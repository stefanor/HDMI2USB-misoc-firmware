#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <console.h>
#include <string.h>
#include <generated/csr.h>
#include <generated/mem.h>
#include <generated/sdram_phy.h>
#include <time.h>

#include "config.h"
#include "fx2.h"
#include "hdmi_in0.h"
#include "hdmi_in1.h"
#include "processor.h"
#include "heartbeat.h"
#include "pll.h"
#include "ci.h"
#include "encoder.h"
#include "opsis_eeprom.h"
#include "tofe_eeprom.h"
#include "hdmi_out0.h"
#include "hdmi_out1.h"
#include "version.h"

int status_enabled;

static void help_video_matrix(void)
{
	wputs("video_matrix commands (alias: 'x')");
	wputs("  video_matrix list              - list available video sinks and sources");
	wputs("  x l                            - list available video sinks and sources");
	wputs("  video_matrix connect <source>  - connect video source to video sink");
	wputs("                       <sink>");
	wputs("  x c <source> <sink>            - connect video source to video sink");
}

static void help_video_mode(void)
{
	wputs("video_mode commands (alias: 'm')");
	wputs("  video_mode list                - list available video modes");
	wputs("  m l                            - list available video modes");
	wputs("  video_mode <mode>              - select video mode");
}

static void help_hdp_toggle(void)
{
	wputs("hdp_toggle <source>              - toggle HDP on source for EDID rescan");
}

static void help_status(void)
{
	wputs("status commands (alias: 's')");
	wputs("  status                         - print status message once");
	wputs("  status <on/off>                - repeatedly print status message");
}

static void help_heartbeat(void)
{
	wputs("change heartbeat status (alias: 'h')");
	wputs("  heartbeat <on/off>             - Turn on/off heartbeat feature");
}

#ifdef CSR_HDMI_OUT0_BASE
static void help_output0(void)
{
	wputs("output0 commands (alias: '0')");
	wputs("  output0 on                     - enable output0");
	wputs("  output0 off                    - disable output0");
}
#endif

#ifdef CSR_HDMI_OUT1_BASE
static void help_output1(void)
{
	wputs("output1 commands (alias: '1')");
	wputs("  output1 on                     - enable output1");
	wputs("  output1 off                    - disable output1");
}
#endif

#ifdef ENCODER_BASE
static void help_encoder(void)
{
	wputs("encoder commands (alias: 'e')");
	wputs("  encoder on                     - enable encoder");
	wputs("  encoder off                    - disable encoder");
	wputs("  encoder quality <quality>      - select quality");
	wputs("  encoder fps <fps>              - configure target fps");
}
#endif

static void help_debug(void)
{
    wputs("debug commands (alias 'd')");
	wputs("  debug pll                      - dump pll configuration");
	wputs("  debug ddr                      - show DDR bandwidth");
	wputs("  debug dna                      - show Board's DNA");
	wputs("  debug edid                     - dump monitor EDID");
	wputs("  debug freq                     - get input frequencies");
#ifdef CSR_OPSIS_EEPROM_I2C_W_ADDR
	wputs("  debug opsis_eeprom             - dump Opsis Info EEPROM");
#endif
#ifdef CSR_TOFE_EEPROM_I2C_W_ADDR
	wputs("  debug tofe_eeprom              - dump TOFE Board Info EEPROM");
#endif
#ifdef CSR_FX2_RESET_OUT_ADDR
	wputs("  debug fx2_reboot firmware      - reboot the FX2 USB IC into firmware");
#endif
#ifdef CSR_CAS_BASE
	wputs("  debug cas leds <value>         - change the status LEDs");
	wputs("  debug cas switches             - read the control switches status");
	wputs("  debug cas buttons read         - read the control buttons status");
	wputs("  debug cas buttons clear        - clear any asserted buttons status");
#endif
}

static void help(void)
{
	wputs("help                             - this command");
	wputs("version                          - firmware/gateware version");
	wputs("reboot                           - reboot CPU");
	wputs("");
	help_status();
	wputs("");
	help_video_matrix();
	wputs("");
	help_video_mode();
	wputs("");
	help_heartbeat();
	wputs("");
	help_hdp_toggle();
	wputs("");
#ifdef CSR_HDMI_OUT0_BASE
	help_output0();
	wputs("");
#endif
#ifdef CSR_HDMI_OUT1_BASE
	help_output1();
	wputs("");
#endif
#ifdef ENCODER_BASE
	help_encoder();
	wputs("");
#endif
	help_debug();
}

static void reboot(void)
{
	asm("call r0");
}

static void status_enable(void)
{
	wprintf("Enabling status\r\n");
	status_enabled = 1;
#ifdef ENCODER_BASE
	encoder_bandwidth_nbytes_clear_write(1);
#endif
}

static void status_disable(void)
{
	wprintf("Disabling status\r\n");
	status_enabled = 0;
}

static void debug_ddr(void);

static void status_print(void)
{
#ifdef CSR_HDMI_IN0_BASE
	wprintf(
		"input0:  %dx%d",
		hdmi_in0_resdetection_hres_read(),
		hdmi_in0_resdetection_vres_read());
#ifdef CSR_HDMI_IN0_FREQUENCY_VALUE_ADDR
	wprintf(" (@ %d kHz)", hdmi_in0_frequency_value_read() / 1000);
#endif
	wprintf("\r\n");
#endif

#ifdef CSR_HDMI_IN1_BASE
	wprintf(
		"input1:  %dx%d",
		hdmi_in1_resdetection_hres_read(),
		hdmi_in1_resdetection_vres_read());
#ifdef CSR_HDMI_IN1_FREQUENCY_VALUE_ADDR
	wprintf(" (@ %d kHz)", hdmi_in1_frequency_value_read() / 1000);
#endif
	wprintf("\r\n");
#endif

#ifdef CSR_HDMI_OUT0_BASE
	wprintf("output0: ");
	if(hdmi_out0_fi_enable_read())
		wprintf(
			"%dx%d@%dHz from %s",
			processor_h_active,
			processor_v_active,
			processor_refresh,
			processor_get_source_name(processor_hdmi_out0_source));
	else
		wprintf("off");
	wprintf("\r\n");
#endif

#ifdef CSR_HDMI_OUT1_BASE
	wprintf("output1: ");
	if(hdmi_out1_fi_enable_read())
		wprintf(
			"%dx%d@%uHz from %s",
			processor_h_active,
			processor_v_active,
			processor_refresh,
			processor_get_source_name(processor_hdmi_out1_source));
	else
		wprintf("off");
	wprintf("\r\n");
#endif

#ifdef ENCODER_BASE
	wprintf("encoder: ");
	if(encoder_enabled) {
		wprintf(
			"%dx%d @ %dfps (%dMbps) from %s (q: %d)",
			processor_h_active,
			processor_v_active,
			encoder_fps,
			encoder_bandwidth_nbytes_read()*8/1000000,
			processor_get_source_name(processor_encoder_source),
			encoder_quality);
		encoder_bandwidth_nbytes_clear_write(1);
	} else
		wprintf("off");
	wprintf("\r\n");
#endif
	wprintf("ddr: ");
	debug_ddr();
}

static void status_service(void)
{
	static int last_event;

	if(elapsed(&last_event, identifier_frequency_read())) {
		if(status_enabled) {
			status_print();
			wprintf("\r\n");
		}
	}
}

static void heartbeat_enable(void)
{
	hb_status(true);
	wprintf("Heartbeat enabled\r\n");

}

static void heartbeat_disable(void)
{
	hb_status(false);
	wprintf("Heartbeat disabled\r\n");

}

static void video_matrix_list(void)
{
	wprintf("Video sources:\r\n");
#ifdef CSR_HDMI_IN0_BASE
	wprintf("input0 (0): %s\r\n", HDMI_IN0_MNEMONIC);
	wputs(HDMI_IN0_DESCRIPTION);
#endif
#ifdef CSR_HDMI_IN1_BASE
	wprintf("input1 (1): %s\r\n", HDMI_IN1_MNEMONIC);
	wputs(HDMI_IN1_DESCRIPTION);
#endif
	wprintf("pattern (p):\r\n");
	wprintf("  Video pattern\r\n");
	wputs(" ");
	wprintf("Video sinks:\r\n");
#ifdef CSR_HDMI_OUT0_BASE
	wprintf("output0 (0): %s\r\n", HDMI_OUT0_MNEMONIC);
	wputs(HDMI_OUT0_DESCRIPTION);
#endif
#ifdef CSR_HDMI_OUT1_BASE
	wprintf("output1 (1): %s\r\n", HDMI_OUT1_MNEMONIC);
	wputs(HDMI_OUT1_DESCRIPTION);
#endif
#ifdef ENCODER_BASE
	wprintf("encoder (e):\r\n");
	wprintf("  JPEG encoder (USB output)\r\n");
#endif
	wputs(" ");
}

static void video_matrix_connect(int source, int sink)
{
	if(source >= 0 && source <= VIDEO_IN_PATTERN)
	{
		if(sink >= 0 && sink <= VIDEO_OUT_HDMI_OUT1) {
			wprintf("Connecting %s to output%d\r\n", processor_get_source_name(source), sink);
			if(sink == VIDEO_OUT_HDMI_OUT0)
#ifdef CSR_HDMI_OUT0_BASE
				processor_set_hdmi_out0_source(source);
#else
				wprintf("hdmi_out0 is missing.\r\n");
#endif
			else if(sink == VIDEO_OUT_HDMI_OUT1)
#ifdef CSR_HDMI_OUT1_BASE
				processor_set_hdmi_out1_source(source);
#else
				wprintf("hdmi_out1 is missing.\r\n");
#endif
			processor_update();
		}
#ifdef ENCODER_BASE
		else if(sink == VIDEO_OUT_ENCODER) {
			wprintf("Connecting %s to encoder\r\n", processor_get_source_name(source));
			processor_set_encoder_source(source);
			processor_update();
		}
#endif
	}
}

static void video_mode_list(void)
{
	char mode_descriptors[PROCESSOR_MODE_COUNT*PROCESSOR_MODE_DESCLEN];
	int i;

	processor_list_modes(mode_descriptors);
	wprintf("Available video modes:\r\n");
	for(i=0;i<PROCESSOR_MODE_COUNT;i++)
		wprintf("mode %d: %s\r\n", i, &mode_descriptors[i*PROCESSOR_MODE_DESCLEN]);
	wprintf("\r\n");
}

static void video_mode_set(int mode)
{
	char mode_descriptors[PROCESSOR_MODE_COUNT*PROCESSOR_MODE_DESCLEN];
	if(mode < PROCESSOR_MODE_COUNT) {
		processor_list_modes(mode_descriptors);
		wprintf("Setting video mode to %s\r\n", &mode_descriptors[mode*PROCESSOR_MODE_DESCLEN]);
		config_set(CONFIG_KEY_RESOLUTION, mode);
		processor_start(mode);
	}
}

static void hdp_toggle(int source)
{
#if defined(CSR_HDMI_IN0_BASE) || defined(CSR_HDMI_IN1_BASE)
	int i;
#endif
	wprintf("Toggling HDP on output%d\r\n", source);
#ifdef CSR_HDMI_IN0_BASE
	if(source ==  VIDEO_IN_HDMI_IN0) {
		hdmi_in0_edid_hpd_en_write(0);
		for(i=0; i<65536; i++);
		hdmi_in0_edid_hpd_en_write(1);
	}
#else
	wprintf("hdmi_in0 is missing.\r\n");
#endif
#ifdef CSR_HDMI_IN1_BASE
	if(source == VIDEO_IN_HDMI_IN1) {
		hdmi_in1_edid_hpd_en_write(0);
		for(i=0; i<65536; i++);
		hdmi_in1_edid_hpd_en_write(1);
	}
#else
	wprintf("hdmi_in1 is missing.\r\n");
#endif
}

#ifdef CSR_HDMI_OUT0_BASE
static void output0_on(void)
{
	wprintf("Enabling output0\r\n");
	hdmi_out0_fi_enable_write(1);
}

static void output0_off(void)
{
	wprintf("Disabling output0\r\n");
	hdmi_out0_fi_enable_write(0);
}
#endif

#ifdef CSR_HDMI_OUT1_BASE
static void output1_on(void)
{
	wprintf("Enabling output1\r\n");
	hdmi_out1_fi_enable_write(1);
}

static void output1_off(void)
{
	wprintf("Disabling output1\r\n");
	hdmi_out1_fi_enable_write(0);
}
#endif

#ifdef ENCODER_BASE
static void encoder_on(void)
{
	wprintf("Enabling encoder\r\n");
	encoder_enable(1);
}

static void encoder_configure_quality(int quality)
{
	wprintf("Setting encoder quality to %d\r\n", quality);
	encoder_set_quality(quality);
}

static void encoder_configure_fps(int fps)
{
	wprintf("Setting encoder fps to %d\r\n", fps);
	encoder_set_fps(fps);
}

static void encoder_off(void)
{
	wprintf("Disabling encoder\r\n");
	encoder_enable(0);
}
#endif

static void debug_pll(void)
{
	pll_dump();
}

static unsigned int log2(unsigned int v)
{
	unsigned int r = 0;
	while(v>>=1) r++;
	return r;
}

static void debug_ddr(void)
{
	unsigned long long int nr, nw;
	unsigned long long int f;
	unsigned int rdb, wrb;
	unsigned int burstbits;

	sdram_controller_bandwidth_update_write(1);
	nr = sdram_controller_bandwidth_nreads_read();
	nw = sdram_controller_bandwidth_nwrites_read();
	f = identifier_frequency_read();
	burstbits = (2*DFII_NPHASES) << DFII_PIX_DATA_SIZE;
	rdb = (nr*f >> (24 - log2(burstbits)))/1000000ULL;
	wrb = (nw*f >> (24 - log2(burstbits)))/1000000ULL;
	wprintf("read:%5dMbps  write:%5dMbps  all:%5dMbps\r\n", rdb, wrb, rdb + wrb);
}

#if defined(CSR_HDMI_IN0_BASE) || defined(CSR_HDMI_IN1_BASE)
static void debug_input(unsigned int channels, unsigned int change, unsigned int state)
{
#ifdef CSR_HDMI_IN0_BASE
    if (channels & 1) {
		if (change & 1)
			hdmi_in0_debug = (state & 1) != 0;
		wprintf("HDMI Input 0 debug %s\r\n", hdmi_in0_debug ? "on" : "off");
	}
#endif
#ifdef CSR_HDMI_IN1_BASE
	if (channels & 2) {
		if (change & 2)
			hdmi_in1_debug = (state & 2) != 0;
		wprintf("HDMI Input 1 debug %s\r\n", hdmi_in0_debug ? "on" : "off");
	}
#endif
}
#endif

static void debug_freq(void)
{
#ifdef CSR_HDMI_IN0_FREQUENCY_VALUE_ADDR
	int hdmi0_freq = hdmi_in0_frequency_value_read();
	int hdmi0_mhz = hdmi0_freq/((int)1e6);
	wprintf("HDMI Input 0 Frequency: %d MHz (%d Hz)\n", hdmi0_mhz, hdmi0_freq);
#endif
#ifdef CSR_HDMI_IN1_FREQUENCY_VALUE_ADDR
	int hdmi1_freq = hdmi_in1_frequency_value_read();
	int hdmi1_mhz = hdmi1_freq/((int)1e6);
	wprintf("HDMI Input 1 Frequency: %d MHz (%d Hz)\n", hdmi1_mhz, hdmi1_freq);
#endif
}

static char *readstr(void)
{
	char c[2];
	static char s[64];
	static int ptr = 0;

	if(readchar_nonblock()) {
		c[0] = readchar();
		c[1] = 0;
		switch(c[0]) {
			case 0x7f:
			case 0x08:
				if(ptr > 0) {
					ptr--;
					wputsnonl("\x08 \x08");
				}
				break;
			case 0x07:
				break;
			case '\r':
			case '\n':
				s[ptr] = 0x00;
				wputsnonl("\r\n");
				ptr = 0;
				return s;
			default:
				if(ptr >= (sizeof(s) - 1))
					break;
				wputsnonl(c);
				s[ptr] = c[0];
				ptr++;
				break;
		}
	}
	return NULL;
}

static char *get_token(char **str)
{
	char *c, *d;

	c = (char *)strchr(*str, ' ');
	if(c == NULL) {
		d = *str;
		*str = *str+strlen(*str);
		return d;
	}
	*c = 0;
	d = *str;
	*str = c+1;
	return d;
}


void ci_prompt(void)
{
	wprintf("HDMI2USB>");
}

void ci_service(void)
{
	char *str;
	char *token;

	status_service();

	str = readstr();
	if(str == NULL) return;

	token = get_token(&str);

	if(strcmp(token, "help") == 0) {
		wputs("Available commands:");
		token = get_token(&str);
		if(strcmp(token, "video_matrix") == 0)
			help_video_matrix();
		else if(strcmp(token, "video_mode") == 0)
			help_video_mode();
		else if(strcmp(token, "hdp_toggle") == 0)
			help_hdp_toggle();
#ifdef CSR_HDMI_OUT0_BASE
		else if(strcmp(token, "output0") == 0)
			help_output0();
#endif
#ifdef CSR_HDMI_OUT1_BASE
		else if(strcmp(token, "output1") == 0)
			help_output1();
#endif
#ifdef ENCODER_BASE
		else if(strcmp(token, "encoder") == 0)
			help_encoder();
#endif
		else if(strcmp(token, "debug") == 0)
			help_debug();

		else if(strcmp(token, "heartbeat") == 0)
			help_heartbeat();

		else
			help();
		wputs("");
	}
	else if(strcmp(token, "reboot") == 0) reboot();
	else if(strcmp(token, "version") == 0) print_version();
	else if((strcmp(token, "video_matrix") == 0) || (strcmp(token, "x") == 0)) {
		token = get_token(&str);
		if((strcmp(token, "list") == 0) || (strcmp(token, "l") == 0)) {
			video_matrix_list();
		}
		else if((strcmp(token, "connect") == 0) || (strcmp(token, "c") == 0)) {
			int source;
			int sink;
			/* get video source */
			token = get_token(&str);
			source = -1;
			if((strcmp(token, "input0") == 0) || (strcmp(token, "0") == 0)) {
				source = VIDEO_IN_HDMI_IN0;
			}
			else if((strcmp(token, "input1") == 0) || (strcmp(token, "1") == 0)) {
				source = VIDEO_IN_HDMI_IN1;
			}
			else if((strcmp(token, "pattern") == 0) || (strcmp(token, "p") == 0)) {
				source = VIDEO_IN_PATTERN;
			}
			else {
				wprintf("Unknown video source: '%s'\r\n", token);
			}

			/* get video sink */
			token = get_token(&str);
			sink = -1;
			if((strcmp(token, "output0") == 0) || (strcmp(token, "0") == 0)) {
				sink = VIDEO_OUT_HDMI_OUT0;
			}
			else if((strcmp(token, "output1") == 0) || (strcmp(token, "1") == 0)) {
				sink = VIDEO_OUT_HDMI_OUT1;
			}
			else if((strcmp(token, "encoder") == 0) || (strcmp(token, "e") == 0)) {
				sink = VIDEO_OUT_ENCODER;
			}
			else
				wprintf("Unknown video sink: '%s'\r\n", token);

			if (source >= 0 && sink >= 0)
				video_matrix_connect(source, sink);
			else
				help_video_matrix();
		} else {
			help_video_matrix();
		}
	}
	else if((strcmp(token, "video_mode") == 0) || (strcmp(token, "m") == 0)) {
		token = get_token(&str);
		if((strcmp(token, "list") == 0) || (strcmp(token, "l") == 0))
			video_mode_list();
		else
			video_mode_set(atoi(token));
	}
	else if((strcmp(token, "heartbeat") == 0) || (strcmp(token, "h") == 0)) {
		token = get_token(&str);
		if((strcmp(token, "on") == 0) )
			heartbeat_enable();
		else if((strcmp(token, "off") == 0) )
			heartbeat_disable();
		else
			help_heartbeat();
	}
	else if(strcmp(token, "hdp_toggle") == 0) {
		token = get_token(&str);
		hdp_toggle(atoi(token));
	}
#ifdef CSR_HDMI_OUT0_BASE
	else if((strcmp(token, "output0") == 0) || (strcmp(token, "0") == 0)) {
		token = get_token(&str);
		if(strcmp(token, "on") == 0)
			output0_on();
		else if(strcmp(token, "off") == 0)
			output0_off();
		else
			help_output0();
	}
#endif
#ifdef CSR_HDMI_OUT1_BASE
	else if((strcmp(token, "output1") == 0) || (strcmp(token, "1") == 0)) {
		token = get_token(&str);
		if(strcmp(token, "on") == 0)
			output1_on();
		else if(strcmp(token, "off") == 0)
			output1_off();
		else
			help_output1();
	}
#endif
#ifdef ENCODER_BASE
	else if((strcmp(token, "encoder") == 0) || (strcmp(token, "e") == 0)) {
		token = get_token(&str);
		if(strcmp(token, "on") == 0)
			encoder_on();
		else if(strcmp(token, "off") == 0)
			encoder_off();
		else if(strcmp(token, "quality") == 0)
			encoder_configure_quality(atoi(get_token(&str)));
		else if(strcmp(token, "fps") == 0)
			encoder_configure_fps(atoi(get_token(&str)));
		else
			help_encoder();
	}
#endif
	else if((strcmp(token, "status") == 0) || (strcmp(token, "s") == 0)) {
		token = get_token(&str);
		if(strcmp(token, "on") == 0)
			status_enable();
		else if(strcmp(token, "off") == 0)
			status_disable();
		else
			status_print();
	}
	else if((strcmp(token, "debug") == 0) || (strcmp(token, "d") == 0)) {
		token = get_token(&str);
		if(strcmp(token, "pll") == 0)
			debug_pll();
#if defined(CSR_HDMI_IN0_BASE) || defined(CSR_HDMI_IN1_BASE)
		else if(strcmp(token, "input") == 0) {
			token = get_token(&str);
			if(strcmp(token, "on") == 0)
				debug_input(3, 3, 3);
			else if(strcmp(token, "off") == 0)
				debug_input(3, 3, 0);
			else if(strcmp(token, "?") != 0) {
				unsigned int state = 0;
#if defined(CSR_HDMI_IN0_BASE)
				state |= (hdmi_in0_debug ? 0 : 1);
#endif
#if defined(CSR_HDMI_IN1_BASE)
				state |= (hdmi_in1_debug ? 0 : 2);
#endif
				debug_input(3, 3, state);
			} else
				debug_input(3, 0, 0);
		}
		else if(strcmp(token, "on") == 0)
			debug_input(3, 3, 3);
		else if(strcmp(token, "off") == 0)
			debug_input(3, 3, 0);
		else if(strcmp(token, "?") == 0)
			debug_input(3, 0, 0);
#endif
#ifdef CSR_HDMI_IN0_BASE
		else if(strcmp(token, "input0") == 0 || strcmp(token, "0") == 0) {
			token = get_token(&str);
			if(strcmp(token, "on") == 0)
				debug_input(1, 1, 1);
			else if(strcmp(token, "off") == 0)
				debug_input(1, 1, 0);
			else if(strcmp(token, "?") != 0)
				debug_input(1, 1, hdmi_in0_debug ? 0 : 1);
			else
				debug_input(1, 0, 0);
		}
#endif
#ifdef CSR_HDMI_IN1_BASE
		else if(strcmp(token, "input1") == 0 || strcmp(token, "1") == 0) {
			token = get_token(&str);
			if(strcmp(token, "on") == 0)
				debug_input(2, 2, 2);
			else if(strcmp(token, "off") == 0)
				debug_input(2, 2, 0);
			else if(strcmp(token, "?") != 0)
				debug_input(2, 2, hdmi_in1_debug ? 0 : 2);
			else
				debug_input(2, 0, 0);
		}
#endif
		else if(strcmp(token, "ddr") == 0)
			debug_ddr();
		else if(strcmp(token, "dna") == 0)
			print_board_dna();
		else if(strcmp(token, "freq") == 0) {
			debug_freq();
                }
#ifdef CSR_OPSIS_EEPROM_I2C_W_ADDR
		else if(strcmp(token, "opsis_eeprom") == 0) {
			opsis_eeprom_dump();
                }
#endif
#ifdef CSR_TOFE_EEPROM_I2C_W_ADDR
		else if(strcmp(token, "tofe_eeprom") == 0) {
			tofe_eeprom_dump();
                }
#endif
#ifdef CSR_FX2_RESET_OUT_ADDR
		else if(strcmp(token, "fx2_reboot") == 0) {
			token = get_token(&str);
			if(strcmp(token, "usbjtag") == 0) {
				fx2_reboot(FX2FW_USBJTAG);
			}
#ifdef ENCODER_BASE
			else if (strcmp(token, "hdmi2usb") == 0) {
				fx2_reboot(FX2FW_HDMI2USB);
			}
#endif
			else {
				fx2_debug();
			}
                }
#endif
		else if(strcmp(token, "edid") == 0) {
			unsigned int found = 0;
			token = get_token(&str);
#ifdef CSR_HDMI_OUT0_I2C_W_ADDR
			if(strcmp(token, "output0") == 0) {
				found = 1;
				hdmi_out0_print_edid();
			}
#endif
#ifdef CSR_HDMI_OUT1_I2C_W_ADDR
			if(strcmp(token, "output1") == 0) {
				found = 1;
				hdmi_out1_print_edid();
			}
#endif
			if(found == 0)
				wprintf("%s port has no EDID capabilities\r\n", token);
		}
#ifdef CSR_CAS_BASE
		else if(strcmp(token, "cas") == 0) {
			token = get_token(&str);
			if(strcmp(token, "leds") == 0) {
				token = get_token(&str);
				cas_leds_out_write(atoi(token));
			}
			else if(strcmp(token, "switches") == 0) {
				wprintf("%X\r\n", (int)cas_switches_in_read());
			}
			else if(strcmp(token, "buttons") == 0) {
				int status = cas_buttons_ev_status_read();
				int pending = cas_buttons_ev_pending_read();
				wprintf("%X %X\r\n", status, pending);
				token = get_token(&str);
				if(strcmp(token, "clear") == 0) {
					cas_buttons_ev_pending_write(pending);
				}
			}
		}
#endif
		else
			help_debug();
	} 
	else {
		if(status_enabled)
			status_disable();
	}

	ci_prompt();
}
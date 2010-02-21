
	.data
	.global ulo_cfg;
ulo_cfg:
	.string "ULO_CFG"
	.long 0x12340001

	.global no_more_snow
no_more_snow:
	.byte 0x0

	.global automatic_dvd_mode
automatic_dvd_mode:
	.byte 0x1

	.global launch_short
launch_short:
	.byte 0x0
	
	.global uhack_settings0
uhack_settings0:
	.byte 0x0

	.byte 0x0,0x0,0x0,0x0

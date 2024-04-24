idf_component_register(
	SRCS
		"mdns_example_main.c" 
		"custom_mbcrc.c" 
		"csv_sd.c"
		"all_variables.c" 
		"sd_card.c"
		"NTP_alls.c" 
		"rs485_custom.c" 
		"heartbeat.c"
		
	INCLUDE_DIRS 
		"."
	
	EMBED_TXTFILES web/index.html
	EMBED_TXTFILES eita.jpg
)



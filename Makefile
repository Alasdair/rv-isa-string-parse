
rv-isa-parse: src/rv_isa_string_parser.c src/rv_isa_string_parser.h
	gcc test.c src/rv_isa_string_parser.c -I src -o rv-isa-parse

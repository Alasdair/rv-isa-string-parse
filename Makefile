.PHONY: clean

rv-isa-parse: src/rv_isa_string_parser.c src/rv_isa_string_parser.h Makefile
	gcc test.c src/rv_isa_string_parser.c -I src -o $@

clean:
	rm rv-isa-parse

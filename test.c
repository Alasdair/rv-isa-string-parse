// Copyright 2023 Alasdair Armstrong
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the “Software”), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE

#include <stdio.h>

#include "src/rv_isa_string_parser.h"

#define DEFAULT_MAJOR_VERSION 2

int main(int argc, char *argv[]) {
  rv_isa_t isa;
  rv_isa_parse_error_t err;

  if (argc < 2) {
    fprintf(stderr, "No ISA string supplied\n");
    return 1;
  }

  if (!rv_isa_string_parse(argv[1], &isa, DEFAULT_MAJOR_VERSION, 0, &err)) {
    fprintf(stderr, "Failed to parse ISA string starting at \"%s\"\n", err.location);
    switch (err.reason) {
    case RV_ISA_AMBIGUOUS_P:
      fprintf(stderr, "Ambigious 'P', could be P extension or minor version separator\n");
      break;
    case RV_ISA_INCORRECT_EXT_ORDER:
      fprintf(stderr, "Extensions must appear in the correct order\n");
      break;
    case RV_ISA_INVALID_EXT_NAME:
      fprintf(stderr, "Invalid extension name\n");
      break;
    case RV_ISA_INVALID_XLEN:
      fprintf(stderr, "Invalid XLEN\n");
      break;
    case RV_ISA_NO_MINOR_VERSION:
      fprintf(stderr, "Expected minor version\n");
      break;
    case RV_ISA_UNSEPARATED_MULTI_LETTER:
      fprintf(stderr, "Multi-letter extension names must be separated by a '_' when following another\n");
      break;
    }
    return 1;
  }
  printf("Found %ld extensions\n", isa.num_exts);
  isa.exts = malloc(sizeof(rv_isa_ext_t) * isa.num_exts);
  rv_isa_string_parse(argv[1], &isa, DEFAULT_MAJOR_VERSION, isa.num_exts, &err);

  for (size_t i = 0; i < isa.num_exts; i++) {
    printf("Extension ");
    fwrite(isa.exts[i].name, 1, isa.exts[i].len, stdout);
    printf(" major=%ld minor=%ld", isa.exts[i].major, isa.exts[i].minor);
    putchar('\n');
  }

  return 0;
}

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

#ifndef RV_ISA_STRING_PARSER
#define RV_ISA_STRING_PARSER

#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

typedef enum {
  RV32,
  RV64,
  RV128,
} rv_isa_xlen_t;

typedef struct {
  // A pointer into the ISA string where the extension name starts
  char *name;
  // The length of the extension name in characters
  size_t len;
  // The extension major version. For standard extensions the default
  // if absent should be whatever the current version of the RISC-V
  // ISA manuals is. For non-standard extensions anything goes. For
  // this reason, we return -1 when this is absent and the consumer
  // can handle the correct per-extension logic.
  long major;
  // The extension minor version, or 0 if absent
  long minor;
} rv_isa_ext_t;

typedef struct {
  rv_isa_xlen_t xlen;
  // The number of explicit extensions found in the ISA string.
  size_t num_exts;
  // A sequence of at most num_exts extensions
  rv_isa_ext_t *exts;
} rv_isa_t;

typedef enum {
  RV_ISA_AMBIGUOUS_P,
  RV_ISA_INCORRECT_EXT_ORDER,
  RV_ISA_INVALID_EXT_NAME,
  RV_ISA_INVALID_XLEN,
  RV_ISA_NO_MINOR_VERSION,
  RV_ISA_UNSEPARATED_MULTI_LETTER,
} rv_isa_parse_error_reason_t;

typedef struct {
  rv_isa_parse_error_reason_t reason;
  char *location;
} rv_isa_parse_error_t;

// Parse a RISC-V ISA string
//
// This function does the job of chopping up the ISA string into its
// various extension components, aiming to fully follow the rules
// specified in the RISC-V ISA manual. It does not implement any logic
// for handling extensions that imply other extensions.
//
// Returns true if the provided string is a valid RISC-V ISA string,
// and false otherwise. When it returns true it will fill the isa
// pointer with information about the parsed extensions. Only max_exts
// extensions will be placed into the exts field of the riscv_isa_t
// struct. If called with max_exts=0, it will still report the number
// of extensions in the num_exts field, so it can be called like:
//
// rv_isa_t isa;
// rv_isa_parse_error_t err;
// if (!rv_isa_string_parse("RV32IMAC", &isa, 2, 0, &err)) {
//   // handle error
// }
// rv_isa_string_parse("RV32IMAC, &isa, 2, isa.num_exts, &err);
//
// to first determine the number of extensions before parsing them.
// Nothing should be assumed about isa contents when the function
// returns false.
//
// If it returns false, information regarding the parse error will be
// placed in the err struct.
bool rv_isa_string_parse(char *isa_string,
                         rv_isa_t *isa,
                         size_t max_exts,
                         rv_isa_parse_error_t *err);

#endif

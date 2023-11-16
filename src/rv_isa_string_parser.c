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

#include "rv_isa_string_parser.h"

bool rv_isa_parse_error(char **isa_string, rv_isa_parse_error_reason_t reason, rv_isa_parse_error_t *err)
{
  err->reason = reason;
  err->location = *isa_string;
  return false;
}

bool rv_isa_string_consume(char **isa_string, size_t n, char *cmp)
{
  for (size_t i = 0; i < n; i++) {
    char c = (cmp++)[0];
    if (tolower((*isa_string)[i]) != c) {
      return false;
    }
  }
  *isa_string += n;
  return true;
}

// Parse a sequence of extensions in a RISC-V ISA string
bool rv_isa_string_parse_exts(char **isa_string,
                              rv_isa_t *isa,
                              long default_major_version,
                              size_t max_exts,
                              rv_isa_parse_error_t *err)
{
  // These are the list of single-character extension names, in the
  // order they appear in the 27.11 table, which is also the order
  // they must appear in the ISA string.
  char *single_char_exts = "iemafdgqlcbjtpvn";

  // The extension we are currently parsing
  rv_isa_ext_t ext;

  char c;

  long major, minor;
  char *major_end;
  char *minor_end;

  // If the last extension we parsed has a version number, this is true
  bool parsed_version = false;
  // True if the current extension is preceeded by an underscore
  bool have_underscore = false;
  // The last single char extension we parsed
  int last_single_char_ext = -1;
  // True if the last extension we parsed was multi-letter
  bool multi_letter = false;

  // The following implements a simple state machine with 3 states
  //
  // * parse_ext_start - start parsing an extension name
  // * parse_version   - start parsing a version number
  // * parse_ext_end   - finish parsing an extension
  //
  // These are implemented using goto, but to avoid any confusing
  // control flow, we ensure each 'state' in the machine either
  // returns or finishes with a goto to another state, so they do not
  // 'fall through' into each other.
parse_ext_start:
  c = tolower((*isa_string)[0]);

  if (c == '\0') {
    return true;
  } else if (c == '_') {
    *isa_string += 1;
    have_underscore = true;
    c = tolower((*isa_string)[0]);
  };

  ext = (rv_isa_ext_t){ .name = NULL, .len = 0, .major = default_major_version, .minor = 0 };

  for (int i = 0; i < (int)strlen(single_char_exts); i++) {
    if (c == single_char_exts[i]) {
      // If we see the 'p' extension after parsing a version number,
      // it must be proceeded by an underscore
      if (c == 'p' && parsed_version && !have_underscore) {
        return rv_isa_parse_error(isa_string, RV_ISA_AMBIGUOUS_P, err);
      }
      // The single char extensions must appear in the correct order
      if (i <= last_single_char_ext) {
        return rv_isa_parse_error(isa_string, RV_ISA_INCORRECT_EXT_ORDER, err);
      }

      ext.name = *isa_string;
      ext.len = 1;

      *isa_string += 1;

      last_single_char_ext = i;
      multi_letter = false;
      goto parse_version;
    }
  }

  if (c == 'z' || c == 's' || c == 'h' || c == 'x') {
    // A multi-letter extension following another must be separated by an underscore
    if (multi_letter && !have_underscore) {
      return rv_isa_parse_error(isa_string, RV_ISA_UNSEPARATED_MULTI_LETTER, err);
    }

    char *ext_name_start = *isa_string;
    size_t ext_name_len = 1;

    *isa_string += 1;

    // We need to handle the svN address translation extensions
    // specially, so we don't treat N as a version number.
    if (c == 's' && ext_name_start[ext_name_len] == 'v' && isdigit(ext_name_start[ext_name_len + 1])) {
      ext_name_len += 1;
      *isa_string += 1;
      while (isdigit(ext_name_start[ext_name_len])) {
        ext_name_len += 1;
        *isa_string += 1;
      }

      ext.name = ext_name_start;
      ext.len = ext_name_len;

      multi_letter = true;
      // Skip version number parsing
      goto parse_ext_end;
    }

    while (isalnum(ext_name_start[ext_name_len])) {
      ext_name_len += 1;
      *isa_string += 1;
    }

    // Now we go backwards removing anything that looks like a version number
    while (isdigit(ext_name_start[ext_name_len - 1])) {
      ext_name_len -= 1;
      *isa_string -= 1;
    }
    if (ext_name_start[ext_name_len - 1] == 'p' && isdigit(ext_name_start[ext_name_len - 2])) {
      ext_name_len -= 1;
      *isa_string -= 1;
      while (isdigit(ext_name_start[ext_name_len - 1])) {
        ext_name_len -= 1;
        *isa_string -= 1;
      }
    }

    ext.name = ext_name_start;
    ext.len = ext_name_len;

    multi_letter = true;
    goto parse_version;
  }

  // If we haven't parsed a valid extension name, then we've failed
  return rv_isa_parse_error(isa_string, RV_ISA_INVALID_EXT_NAME, err);

  // Parse a version number if it exists
parse_version:
  major = strtol(*isa_string, &major_end, 10);

  if (*isa_string == major_end) {
    parsed_version = false;
    goto parse_ext_end;
  }

  ext.major = major;
  *isa_string = major_end;

  // Now check for a minor version number
  if ((*isa_string)[0] != 'p') {
    parsed_version = true;
    goto parse_ext_end;
  }
  *isa_string += 1;

  minor = strtol(*isa_string, &minor_end, 10);

  if (*isa_string != minor_end) {
    ext.minor = minor;
    *isa_string = minor_end;

    parsed_version = true;
    goto parse_ext_end;
  }

  // If we saw 'p', then we must parse a minor version number
  return rv_isa_parse_error(isa_string, RV_ISA_NO_MINOR_VERSION, err);

parse_ext_end:
  if (isa->num_exts < max_exts) {
    isa->exts[isa->num_exts] = ext;
  }
  isa->num_exts++;

  goto parse_ext_start;
}

bool rv_isa_string_parse(char *isa_string,
                         rv_isa_t *isa,
                         long default_major_version,
                         size_t max_exts,
                         rv_isa_parse_error_t *err)
{
  isa->num_exts = 0;

  if (rv_isa_string_consume(&isa_string, 4, "rv32")) {
    isa->xlen = RV32;
  } else if (rv_isa_string_consume(&isa_string, 4, "rv64")) {
    isa->xlen = RV64;
  } else if (rv_isa_string_consume(&isa_string, 5, "rv128")) {
    isa->xlen = RV128;
  } else {
    return rv_isa_parse_error(&isa_string, RV_ISA_INVALID_XLEN, err);
  }

  return rv_isa_string_parse_exts(&isa_string, isa, default_major_version, max_exts, err);
}

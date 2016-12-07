  // reserved words in C
  cnames.insert(astr("auto"));
  cnames.insert(astr("bool"));
  cnames.insert(astr("break"));
  cnames.insert(astr("case"));
  cnames.insert(astr("char"));
  cnames.insert(astr("const"));
  cnames.insert(astr("continue"));
  cnames.insert(astr("default"));
  cnames.insert(astr("do"));
  cnames.insert(astr("double"));
  cnames.insert(astr("else"));
  cnames.insert(astr("enum"));
  cnames.insert(astr("extern"));
  cnames.insert(astr("float"));
  cnames.insert(astr("for"));
  cnames.insert(astr("goto"));
  cnames.insert(astr("if"));
  cnames.insert(astr("inline"));
  cnames.insert(astr("int"));
  cnames.insert(astr("long"));
  cnames.insert(astr("register"));
  cnames.insert(astr("restrict"));
  cnames.insert(astr("return"));
  cnames.insert(astr("short"));
  cnames.insert(astr("signed"));
  cnames.insert(astr("sizeof"));
  cnames.insert(astr("static"));
  cnames.insert(astr("struct"));
  cnames.insert(astr("switch"));
  cnames.insert(astr("typedef"));
  cnames.insert(astr("union"));
  cnames.insert(astr("unsigned"));
  cnames.insert(astr("void"));
  cnames.insert(astr("volatile"));
  cnames.insert(astr("while"));
  // all symbol names beginning with _ followed by an uppercase letter
  // need to be mangled; this should be done with custom code

  cnames.insert(astr("abs"));
  cnames.insert(astr("cos"));
  cnames.insert(astr("sin"));
  cnames.insert(astr("tan"));
  cnames.insert(astr("acos"));
  cnames.insert(astr("asin"));
  cnames.insert(astr("atan"));
  cnames.insert(astr("chdir"));
  cnames.insert(astr("chown"));
  cnames.insert(astr("cwd"));
  cnames.insert(astr("floor"));
  cnames.insert(astr("sqrt"));
  cnames.insert(astr("conjg"));
  cnames.insert(astr("exit"));
  cnames.insert(astr("_init"));
  cnames.insert(astr("stdin"));
  cnames.insert(astr("close"));
  cnames.insert(astr("fwrite"));
  cnames.insert(astr("fread"));
  cnames.insert(astr("fsync"));
  cnames.insert(astr("main"));
  cnames.insert(astr("open"));
  cnames.insert(astr("pow"));
  cnames.insert(astr("printf"));
  cnames.insert(astr("remove"));
  cnames.insert(astr("rename"));
  cnames.insert(astr("scanf"));
  cnames.insert(astr("sync"));
  cnames.insert(astr("quad"));
  cnames.insert(astr("read"));
  cnames.insert(astr("sleep"));
  cnames.insert(astr("stderr"));
  cnames.insert(astr("stdout"));
  cnames.insert(astr("wait"));
  cnames.insert(astr("write"));
  cnames.insert(astr("j0")); // the following 6 are in /usr/include/bits/mathcalls.h - ridiculous
  cnames.insert(astr("j1"));
  cnames.insert(astr("jn"));
  cnames.insert(astr("y0"));
  cnames.insert(astr("y1")); // this is ridiculous...
  cnames.insert(astr("yn"));
  cnames.insert(astr("log2"));
  cnames.insert(astr("remove"));
  cnames.insert(astr("fprintf"));
  cnames.insert(astr("fscanf"));
  cnames.insert(astr("clone"));
  cnames.insert(astr("new"));
  cnames.insert(astr("delete"));
  cnames.insert(astr("register"));
  cnames.insert(astr("signal"));
  cnames.insert(astr("ceil"));
  cnames.insert(astr("acosh"));
  cnames.insert(astr("asinh"));
  cnames.insert(astr("atan2"));
  cnames.insert(astr("atanh"));
  cnames.insert(astr("cbrt"));
  cnames.insert(astr("cosh"));
  cnames.insert(astr("erf"));
  cnames.insert(astr("erfc"));
  cnames.insert(astr("exp"));
  cnames.insert(astr("exp2"));
  cnames.insert(astr("expm1"));
  cnames.insert(astr("lgamma"));
  cnames.insert(astr("log10"));
  cnames.insert(astr("log1p"));
  cnames.insert(astr("log"));
  cnames.insert(astr("rint"));
  cnames.insert(astr("sinh"));
  cnames.insert(astr("tanh"));
  cnames.insert(astr("ascii"));
  cnames.insert(astr("isnan"));
  cnames.insert(astr("random"));
  cnames.insert(astr("truncate"));
  cnames.insert(astr("time"));
  cnames.insert(astr("nearbyint"));
  cnames.insert(astr("round"));
  cnames.insert(astr("tgamma"));
  cnames.insert(astr("trunc"));
  cnames.insert(astr("div"));
  cnames.insert(astr("max"));
  cnames.insert(astr("min"));
  cnames.insert(astr("malloc"));
  cnames.insert(astr("free"));

  cnames.insert(astr("unlink"));

  // symbols in stdint.h
  cnames.insert(astr("int8_t"));
  cnames.insert(astr("int16_t"));
  cnames.insert(astr("int32_t"));
  cnames.insert(astr("int64_t"));
  cnames.insert(astr("uint8_t"));
  cnames.insert(astr("uint16_t"));
  cnames.insert(astr("uint32_t"));
  cnames.insert(astr("uint64_t"));
  cnames.insert(astr("int_least8_t"));
  cnames.insert(astr("int_least16_t"));
  cnames.insert(astr("int_least32_t"));
  cnames.insert(astr("int_least64_t"));
  cnames.insert(astr("uint_least8_t"));
  cnames.insert(astr("uint_least16_t"));
  cnames.insert(astr("uint_least32_t"));
  cnames.insert(astr("uint_least64_t"));
  cnames.insert(astr("int_fast8_t"));
  cnames.insert(astr("int_fast16_t"));
  cnames.insert(astr("int_fast32_t"));
  cnames.insert(astr("int_fast64_t"));
  cnames.insert(astr("uint_fast8_t"));
  cnames.insert(astr("uint_fast16_t"));
  cnames.insert(astr("uint_fast32_t"));
  cnames.insert(astr("uint_fast64_t"));
  cnames.insert(astr("intptr_t"));
  cnames.insert(astr("uintptr_t"));
  cnames.insert(astr("intmax_t"));
  cnames.insert(astr("uintmax_t"));
  cnames.insert(astr("INT8_MAX"));
  cnames.insert(astr("INT16_MAX"));
  cnames.insert(astr("INT32_MAX"));
  cnames.insert(astr("INT64_MAX"));
  cnames.insert(astr("INT8_MIN"));
  cnames.insert(astr("INT16_MIN"));
  cnames.insert(astr("INT32_MIN"));
  cnames.insert(astr("INT64_MIN"));
  cnames.insert(astr("UINT8_MAX"));
  cnames.insert(astr("UINT16_MAX"));
  cnames.insert(astr("UINT32_MAX"));
  cnames.insert(astr("UINT64_MAX"));
  cnames.insert(astr("INT_LEAST8_MIN"));
  cnames.insert(astr("INT_LEAST16_MIN"));
  cnames.insert(astr("INT_LEAST32_MIN"));
  cnames.insert(astr("INT_LEAST64_MIN"));
  cnames.insert(astr("INT_LEAST8_MAX"));
  cnames.insert(astr("INT_LEAST16_MAX"));
  cnames.insert(astr("INT_LEAST32_MAX"));
  cnames.insert(astr("INT_LEAST64_MAX"));
  cnames.insert(astr("UINT_LEAST8_MAX"));
  cnames.insert(astr("UINT_LEAST16_MAX"));
  cnames.insert(astr("UINT_LEAST32_MAX"));
  cnames.insert(astr("UINT_LEAST64_MAX"));
  cnames.insert(astr("INT_FAST8_MIN"));
  cnames.insert(astr("INT_FAST16_MIN"));
  cnames.insert(astr("INT_FAST32_MIN"));
  cnames.insert(astr("INT_FAST64_MIN"));
  cnames.insert(astr("INT_FAST8_MAX"));
  cnames.insert(astr("INT_FAST16_MAX"));
  cnames.insert(astr("INT_FAST32_MAX"));
  cnames.insert(astr("INT_FAST64_MAX"));
  cnames.insert(astr("UINT_FAST8_MAX"));
  cnames.insert(astr("UINT_FAST16_MAX"));
  cnames.insert(astr("UINT_FAST32_MAX"));
  cnames.insert(astr("UINT_FAST64_MAX"));
  cnames.insert(astr("INTPTR_MIN"));
  cnames.insert(astr("INTPTR_MAX"));
  cnames.insert(astr("UINTPTR_MAX"));
  cnames.insert(astr("INTMAX_MIN"));
  cnames.insert(astr("INTMAX_MAX"));
  cnames.insert(astr("UINTMAX_MAX"));
  cnames.insert(astr("PTRDIFF_MIN"));
  cnames.insert(astr("PTRDIFF_MAX"));
  cnames.insert(astr("SIZE_MAX"));
  cnames.insert(astr("WCHAR_MAX"));
  cnames.insert(astr("WCHAR_MIN"));
  cnames.insert(astr("WINT_MIN"));
  cnames.insert(astr("WINT_MAX"));
  cnames.insert(astr("SIG_ATOMIC_MIN"));
  cnames.insert(astr("SIG_ATOMIC_MAX"));
  cnames.insert(astr("INT8_C"));
  cnames.insert(astr("INT16_C"));
  cnames.insert(astr("INT32_C"));
  cnames.insert(astr("INT64_C"));
  cnames.insert(astr("UINT8_C"));
  cnames.insert(astr("UINT16_C"));
  cnames.insert(astr("UINT32_C"));
  cnames.insert(astr("UINT64_C"));
  cnames.insert(astr("INTMAX_C"));
  cnames.insert(astr("UINTMAX_C"));

  // symbols from stdio.h
  cnames.insert(astr("EOF"));
  cnames.insert(astr("puts"));
  cnames.insert(astr("gets"));

  // symbols from QIO
  cnames.insert(astr("EEOF"));
  cnames.insert(astr("ESHORT"));
  cnames.insert(astr("EFORMAT"));

  // reserved words in c++
  cnames.insert(astr("this"));
  cnames.insert(astr("template"));
  cnames.insert(astr("assert"));

  cnames.insert(astr("flag"));

  // New additions from the PGI 13.3 C runtime libpgc.a (relaxedmath.o):
  cnames.insert(astr("alloca"));
  cnames.insert(astr("add128"));
  cnames.insert(astr("add192"));
  cnames.insert(astr("eq128"));
  cnames.insert(astr("le128"));
  cnames.insert(astr("lt128"));
  cnames.insert(astr("ne128"));
  cnames.insert(astr("sub128"));
  cnames.insert(astr("sub192"));
  cnames.insert(astr("a1"));
  cnames.insert(astr("a2"));
  cnames.insert(astr("a3"));
  cnames.insert(astr("inc"));
  cnames.insert(astr("inv32"));
  cnames.insert(astr("invln2"));
  cnames.insert(astr("isign"));
  cnames.insert(astr("ln2"));
  cnames.insert(astr("pi"));
  cnames.insert(astr("table"));
  cnames.insert(astr("ten23"));
  cnames.insert(astr("ts"));
  cnames.insert(astr("two"));

  // reserved on darwin
  cnames.insert(astr("fls"));
  cnames.insert(astr("wait2"));
  cnames.insert(astr("wait3"));
  cnames.insert(astr("wait4"));

  // symbols in wordexp.h
  cnames.insert(astr("wordexp"));

  // symbols in glob.h
  cnames.insert(astr("glob"));

  // symbols in math.h
  cnames.insert(astr("isinf"));
  cnames.insert(astr("isfinite"));
  cnames.insert(astr("NAN"));
  cnames.insert(astr("INFINITY"));

  // symbols from stat.h
  cnames.insert(astr("chmod"));
  cnames.insert(astr("fchmod"));
  cnames.insert(astr("fstat"));
  cnames.insert(astr("lstat"));
  cnames.insert(astr("mkdir"));
  cnames.insert(astr("mkfifo"));
  cnames.insert(astr("mknod"));
  cnames.insert(astr("stat"));
  cnames.insert(astr("symlink"));
  cnames.insert(astr("umask"));

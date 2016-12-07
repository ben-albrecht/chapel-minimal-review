/* The type corresponding to the C int type */
extern type c_int= int(32);
/* The type corresponding to the C uint type */
extern type c_uint= uint(32);
/* The type corresponding to the C long type */
extern type c_long= int(64);
/* The type corresponding to the C unsigned long type */
extern type c_ulong= uint(64);
/* The type corresponding to the C long long type */
extern type c_longlong= int(64);
/* The type corresponding to the C unsigned long long type */
extern type c_ulonglong= uint(64);
/* The type corresponding to the C char type */
extern type c_char= int(8);
/* The type corresponding to the C signed char type */
extern type c_schar= int(8);
/* The type corresponding to the C unsigned char type */
extern type c_uchar= uint(8);
/* The type corresponding to the C short type */
extern type c_short= int(16);
/* The type corresponding to the C unsigned short type */
extern type c_ushort= uint(16);
/* The type corresponding to the C intptr_t type */
extern type c_intptr= int(64);
/* The type corresponding to the C uintptr_t type */
extern type c_uintptr= uint(64);
/* The type corresponding to the C ptrdiff_t type */
extern type c_ptrdiff= int(64);
/* The type corresponding to the C ssize_t type */
extern type ssize_t= int(64);
/* The type corresponding to the C size_t type */
extern type size_t= uint(64);

{
  pragma "no prototype"
  extern proc sizeof(type t): size_t;

  assert(sizeof(c_int) == sizeof(int(32)));
  assert(sizeof(c_uint) == sizeof(uint(32)));
  assert(sizeof(c_long) == sizeof(int(64)));
  assert(sizeof(c_ulong) == sizeof(uint(64)));
  assert(sizeof(c_longlong) == sizeof(int(64)));
  assert(sizeof(c_ulonglong) == sizeof(uint(64)));
  assert(sizeof(c_char) == sizeof(int(8)));
  assert(sizeof(c_schar) == sizeof(int(8)));
  assert(sizeof(c_uchar) == sizeof(uint(8)));
  assert(sizeof(c_short) == sizeof(int(16)));
  assert(sizeof(c_ushort) == sizeof(uint(16)));
  assert(sizeof(c_intptr) == sizeof(int(64)));
  assert(sizeof(c_uintptr) == sizeof(uint(64)));
  assert(sizeof(c_ptrdiff) == sizeof(int(64)));
  assert(sizeof(ssize_t) == sizeof(int(64)));
  assert(sizeof(size_t) == sizeof(uint(64)));
}

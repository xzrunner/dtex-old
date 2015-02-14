#ifdef __cplusplus
extern "C"
{
#endif

#define IS_POT(x) ((x) > 0 && ((x) & ((x) -1)) == 0)

#define TO_4TIMES(x) (((x) + 3) & ~3)
#define IS_4TIMES(x) ((x) % 4 == 0)

inline int next_p2(int a)
{
	int rval = 1;
	while(rval < a) {
		rval <<= 1;
	}
	return rval;
}

#ifdef __cplusplus
}
#endif
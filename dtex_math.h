#ifdef __cplusplus
extern "C"
{
#endif

#define IS_POT(x) (((x) & ((x) -1)) == 0)

#define TO_4TIMES(x) (((x) + 3) & ~3)
#define IS_4TIMES(x) ((x) % 4 == 0)

#ifdef __cplusplus
}
#endif
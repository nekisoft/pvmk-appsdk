//atomics.c
//Stub implementations of atomic functions
//Bryan E. Topp <betopp@betopp.com> 2024

//This is all bullshit because we're single-core single-threaded and usually single-processing.
//Ported software commonly uses a lot of atomic operations though, expecting to be multithreaded.
//So just do it naively to get those operations done without worrying about atomicity.
//(I don't think ARMv5TE even has the right instructions for this to be implemented for real.)

//Type names as gcc libatomic defines them
#define I1 unsigned char
#define I2 unsigned short
#define I4 unsigned int
#define I8 unsigned long long

void __sync_synchronize(void)
{
	//Do nothing
	//This is supposed to make sure all prior memory writes are visible to all cores
	//But we're single-core and single-threaded
}

I1 __atomic_exchange_1(volatile void *mem, I1 val, int model)
{
	(void)model;
	I1 retval = *(I1*)mem;
	*(I1*)mem = val;
	return retval;
}

I2 __atomic_exchange_2(volatile void *mem, I2 val, int model)
{
	(void)model;
	I2 retval = *(I2*)mem;
	*(I2*)mem = val;
	return retval;
}

I4 __atomic_exchange_4(volatile void *mem, I4 val, int model)
{
	(void)model;
	I4 retval = *(I4*)mem;
	*(I4*)mem = val;
	return retval;
}

I8 __atomic_exchange_8(volatile void *mem, I8 val, int model)
{
	(void)model;
	I8 retval = *(I8*)mem;
	*(I8*)mem = val;
	return retval;
}

I1 __atomic_fetch_add_1(volatile void *mem, I1 val, int model)
{
	(void)model;
	I1 old = *(I1*)mem;
	*(I1*)mem += val;
	return old;
}

I2 __atomic_fetch_add_2(volatile void *mem, I2 val, int model)
{
	(void)model;
	I2 old = *(I2*)mem;
	*(I2*)mem += val;
	return old;	
}

I4 __atomic_fetch_add_4(volatile void *mem, I4 val, int model)
{
	(void)model;
	I4 old = *(I4*)mem;
	*(I4*)mem += val;
	return old;		
}

I8 __atomic_fetch_add_8(volatile void *mem, I8 val, int model)
{
	(void)model;
	I8 old = *(I8*)mem;
	*(I8*)mem += val;
	return old;		
}

I1 __atomic_fetch_and_1(volatile void *mem, I1 val, int model)
{
	(void)model;
	I1 old = *(I1*)mem;
	*(I1*)mem &= val;
	return old;
}

I2 __atomic_fetch_and_2(volatile void *mem, I2 val, int model)
{
	(void)model;
	I2 old = *(I2*)mem;
	*(I2*)mem &= val;
	return old;	
}

I4 __atomic_fetch_and_4(volatile void *mem, I4 val, int model)
{
	(void)model;
	I4 old = *(I4*)mem;
	*(I4*)mem &= val;
	return old;		
}

I1 __atomic_fetch_or_1(volatile void *mem, I1 val, int model)
{
	(void)model;
	I1 old = *(I1*)mem;
	*(I1*)mem |= val;
	return old;
}

I2 __atomic_fetch_or_2(volatile void *mem, I2 val, int model)
{
	(void)model;
	I2 old = *(I2*)mem;
	*(I2*)mem |= val;
	return old;	
}

I4 __atomic_fetch_or_4(volatile void *mem, I4 val, int model)
{
	(void)model;
	I4 old = *(I4*)mem;
	*(I4*)mem |= val;
	return old;		
}

_Bool __atomic_compare_exchange_1(volatile void *mem, void *expected, I1 desired, _Bool strong, int success, int failure)
{
	I1 had_mem = *(I1*)mem;
	I1 had_exp = *(I1*)expected;
	*(I1*)expected = had_mem;
	*(I1*)mem = (had_mem == had_exp)?desired:had_mem;
	
	(void)strong; (void)success; (void)failure; return (had_mem == had_exp);
}

_Bool __atomic_compare_exchange_2(volatile void *mem, void *expected, I2 desired, _Bool strong, int success, int failure)
{
	I2 had_mem = *(I2*)mem;
	I2 had_exp = *(I2*)expected;
	*(I2*)expected = had_mem;
	*(I2*)mem = (had_mem == had_exp)?desired:had_mem;
	
	(void)strong; (void)success; (void)failure; return (had_mem == had_exp);
}

_Bool __atomic_compare_exchange_4(volatile void *mem, void *expected, I4 desired, _Bool strong, int success, int failure)
{
	I4 had_mem = *(I4*)mem;
	I4 had_exp = *(I4*)expected;
	*(I4*)expected = had_mem;
	*(I4*)mem = (had_mem == had_exp)?desired:had_mem;
	
	(void)strong; (void)success; (void)failure; return (had_mem == had_exp);
}

_Bool __atomic_compare_exchange_8(volatile void *mem, void *expected, I8 desired, _Bool strong, int success, int failure)
{
	I8 had_mem = *(I8*)mem;
	I8 had_exp = *(I8*)expected;
	*(I8*)expected = had_mem;
	*(I8*)mem = (had_mem == had_exp)?desired:had_mem;
	
	(void)strong; (void)success; (void)failure; return (had_mem == had_exp);
}

I1 __atomic_load_1(const volatile void *mem, int model)
{
	(void)model;
	return *(I1*)mem;
}

I2 __atomic_load_2(const volatile void *mem, int model)
{
	(void)model;
	return *(I2*)mem;
}

I4 __atomic_load_4(const volatile void *mem, int model)
{
	(void)model;
	return *(I4*)mem;
}

I8 __atomic_load_8(const volatile void *mem, int model)
{
	(void)model;
	return *(I8*)mem;
}

void __atomic_store_1(volatile void *mem, I1 val, int model)
{
	(void)model;
	*(I1*)mem = val;
}

void __atomic_store_2(volatile void *mem, I2 val, int model)
{
	(void)model;
	*(I2*)mem = val;
}

void __atomic_store_4(volatile void *mem, I4 val, int model)
{
	(void)model;
	*(I4*)mem = val;
}

void __atomic_store_8(volatile void *mem, I8 val, int model)
{
	(void)model;
	*(I8*)mem = val;
}


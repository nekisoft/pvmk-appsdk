//rough_float.h
//Rough float routines for machines lacking FPU
//Bryan E. Topp <betopp@betopp.com> 2025
#ifndef ROUGH_FLOAT_H
#define ROUGH_FLOAT_H

//Replacements for aeabi float functions, but shittier

//Rough floating point type - processing with integer ops
//Depends on using IEEE754 (as everybody does anyway)
typedef union rf_u
{
	float f;
	uint32_t i;
	struct { //Depends on the C compiler assigning bits LSBit-first
		uint32_t ma : 23; //Mantissa
		uint32_t ex : 8;  //Exponent
		uint32_t sg : 1;  //Sign
	} p;
} rf_t;

static inline float rf_add(float a, float b)
{	
	rf_t ra = { .f = a };
	rf_t rb = { .f = b };
	
	//Adding close-to-zero to a number does nothing
	if(ra.p.ex < 4)
		return b;
	if(rb.p.ex < 4)
		return a;
	
	if(ra.p.sg == rb.p.sg)
	{
		//Signs are the same - addition
		
		//Extract fixed-point value of mantissa + implied 1
		uint32_t ia = ra.p.ma | 0x800000u; //with implied "1", 1.23
		uint32_t ib = rb.p.ma | 0x800000u; //with implied "1", 1.23
		
		//Match magnitudes - to the bigger one
		uint32_t ea = ra.p.ex;
		uint32_t eb = rb.p.ex;
		if(ea < eb)
		{
			ia >>= (eb - ea);
			ea = eb;
		}
		else if(eb < ea)
		{
			ib >>= (ea - eb);
			eb = ea;
		}
		
		//Add the two and use the bigger magnitude
		uint32_t exp = ea; // == eb
		uint32_t sum = ia + ib;
		
		//Normalize - mantissa (with implied-1) has to end up between 1 and less-than-2
		while(sum >= 0x1000000u)
		{
			//Cut bit from mantissa, add to exponent
			sum >>= 1;
			exp++;
		}
		sum &= 0x7FFFFFu; //chop off implied 1
		
		//Done
		rf_t retval = { .p = { .ma = sum, .ex = exp, .sg = ra.p.sg } };
		return retval.f;
	}
	else
	{
		//Signs are different - subtraction
		
		//Extract fixed-point value of mantissa + implied 1
		uint32_t ia = ra.p.ma | 0x800000u; //with implied "1", 1.23
		uint32_t ib = rb.p.ma | 0x800000u; //with implied "1", 1.23
		
		//Match magnitudes - to the bigger one
		uint32_t ea = ra.p.ex;
		uint32_t eb = rb.p.ex;
		if(ea < eb)
		{
			ia >>= (eb - ea);
			ea = eb;
		}
		else if(eb < ea)
		{
			ib >>= (ea - eb);
			eb = ea;
		}
	
		//Add the two and use the bigger magnitude and its sign
		uint32_t exp = ea; // == eb
		uint32_t diff = (ia > ib) ? (ia - ib) : (ib - ia);
		uint32_t sign = (ia > ib) ? ra.p.sg : rb.p.sg;
		
		//Normalize - mantissa (with implied-1) has to end up between 1 and less-than-2
		if(diff == 0)
		{
			//Flush to zero
			return 0;
		}
	
		while(diff < 0x800000u)
		{
			//Add bit to mantissa, remove value from exponent
			diff <<= 1;
			exp--;
		}
		diff &= 0x7FFFFFu; //chop off implied 1
		
		//Done
		rf_t retval = { .p = { .ma = diff, .ex = exp, .sg = sign } };
		return retval.f;		
	}
}

static inline float rf_sub(float a, float b)
{
	//Flip sign bit on "b" input, use addition function
	rf_t rb = { .f = b };
	rb.p.sg ^= 1;
	
	return rf_add(a, rb.f);
}

static inline float rf_mul(float a, float b)
{
	rf_t ra = { .f = a };
	rf_t rb = { .f = b };

	//Add the exponents
	int expsum = ra.p.ex + rb.p.ex; //add biased exponents
	expsum -= 0x7F; //remove one of the biases
	if(expsum < 20) //flush-to-zero if it's even close
		return 0; 
		
	
	//Multiply the mantissa bits
	uint32_t ma = ra.p.ma | 0x800000u; //1.23
	uint32_t mb = rb.p.ma | 0x800000u; //1.23
	uint64_t manprod64 = (uint64_t)ma * (uint64_t)mb; //2.46

	uint32_t manprod = manprod64 >> 23; //cut down to 23 bits of fraction
	if(manprod >= 0x1000000u) //if we end up with more than "1" as our integer part, shift down
	{
		manprod >>= 1;
		expsum++;
	}
	if(manprod < 0x800000u) //if we end up with less than "1" as our integer part, shift up
	{
		manprod <<= 1;
		expsum--;
	}
	manprod &= 0x7FFFFFu; //whole part is now 1, will be implied, cut it off
	
	//Build the result
	rf_t rr = { .p = { .sg = ra.p.sg ^ rb.p.sg, .ma = manprod, .ex = expsum } };
	return rr.f;
}

static inline float rf_sq(float a)
{
	rf_t ra = { .f = a };

	int expsum = ((ra.i & 0x7F800000u) >> 22); //Double the exponent
	expsum -= 0x7F; //remove one of the biases
	if(expsum < 20) //flush-to-zero if it's even close
		return 0; 
		
	//Multiply the mantissa bits
	uint32_t ma = ra.p.ma | 0x800000u; //1.23
	uint64_t manprod64 = (uint64_t)ma * (uint64_t)ma; //2.46

	uint32_t manprod = manprod64 >> 23; //cut down to 23 bits of fraction
	if(manprod >= 0x1000000u) //if we end up with more than "1" as our integer part, shift down
	{
		manprod >>= 1;
		expsum++;
	}
	if(manprod < 0x800000u) //if we end up with less than "1" as our integer part, shift up
	{
		manprod <<= 1;
		expsum--;
	}
	manprod &= 0x7FFFFFu; //whole part is now 1, will be implied, cut it off
	
	//Build the result
	rf_t rr = { .p = { .sg = 0, .ma = manprod, .ex = expsum } };
	return rr.f;	
}


#endif //ROUGH_FLOAT_H

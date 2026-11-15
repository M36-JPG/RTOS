

#ifndef BIT_MATH_H_
#define BIT_MATH_H_


#define SET_BIT(REG, BIT)       ((REG) |= (1U << (BIT)))
#define CLR_BIT(REG, BIT)       ((REG) &= ~(1U << (BIT)))
#define GET_BIT(REG, BIT)       (((REG) >> (BIT)) & 1U)
#define TGL_BIT(REG, BIT)       ((REG) ^= (1U << (BIT)))


#endif   //BIT_MATH
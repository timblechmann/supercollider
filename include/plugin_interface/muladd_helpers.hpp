//  muladd helper classes, broken out of nova
//
//  Copyright (C) 2010 Tim Blechmann
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; see the file COPYING.  If not, write to
//  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//  Boston, MA 02111-1307, USA.

#ifndef SC_MULADD_HELPERS_HPP
#define SC_MULADD_HELPERS_HPP

/* dummy muladd helper */
template <typename sample_type>
struct muladd_helper_nop
{
    sample_type operator()(sample_type sample)
    {
        return sample;
    }
};

/* muladd helper, constant factor */
template <typename sample_type>
struct muladd_helper_mul_c
{
    muladd_helper_mul_c(sample_type const & factor):
        factor(factor)
    {}

    sample_type operator()(sample_type sample)
    {
        return sample * factor;
    }

    sample_type factor;
};

/* muladd helper, linear changing factor */
template <typename sample_type>
struct muladd_helper_mul_l
{
    muladd_helper_mul_l(sample_type const & factor, sample_type const & slope):
        factor(factor), slope(slope)
    {}

    sample_type operator()(sample_type sample)
    {
        sample_type ret = sample * factor;
        factor += slope;
        return ret;
    }

    sample_type factor;
    sample_type slope;
};

/* muladd helper, factor vector */
template <typename sample_type>
struct muladd_helper_mul_v
{
    muladd_helper_mul_v(sample_type * const & input):
        factors(input)
    {}

    sample_type operator()(sample_type sample)
    {
        return sample * *factors++;
    }

    const sample_type * factors;
};


/* muladd helper, constant offset */
template <typename sample_type>
struct muladd_helper_add_c
{
    muladd_helper_add_c(sample_type const & offset):
        offset(offset)
    {}

    sample_type operator()(sample_type sample)
    {
        return sample + offset;
    }

    sample_type offset;
};

/* muladd helper, linear changing offset */
template <typename sample_type>
struct muladd_helper_add_l
{
    muladd_helper_add_l(sample_type const & offset, sample_type const & slope):
        offset(offset), slope(slope)
    {}

    sample_type operator()(sample_type sample)
    {
        sample_type ret = sample + offset;
        offset += slope;
        return ret;
    }

    sample_type offset;
    sample_type slope;
};

/* muladd helper, offset vector */
template <typename sample_type>
struct muladd_helper_add_v
{
    muladd_helper_add_v(sample_type * const & input):
        offsets(input)
    {}

    sample_type operator()(sample_type sample)
    {
        return sample + *offsets++;
    }

    const sample_type * offsets;
};



/* muladd helper, constant factor, constant offset */
template <typename sample_type>
struct muladd_helper_mul_c_add_c
{
    muladd_helper_mul_c_add_c(sample_type const & factor, sample_type const & offset):
        factor(factor), offset(offset)
    {}

    sample_type operator()(sample_type sample)
    {
        return sample * factor + offset;
    }

    sample_type factor, offset;
};

/* muladd helper, constant factor, linear changing offset */
template <typename sample_type>
struct muladd_helper_mul_c_add_l
{
    muladd_helper_mul_c_add_l(sample_type const & factor, sample_type const & offset,
                              sample_type const & offset_slope):
        factor(factor), offset(offset), offset_slope(offset_slope)
    {}

    sample_type operator()(sample_type sample)
    {
        sample_type ret = sample * factor + offset;
        offset += offset_slope;
        return ret;
    }

    sample_type factor, offset, offset_slope;
};


/* muladd helper, constant factor, offset vector */
template <typename sample_type>
struct muladd_helper_mul_c_add_v
{
    muladd_helper_mul_c_add_v(sample_type const & factor, sample_type * const & offsets):
        factor(factor), offsets( offsets )
    {}

    sample_type operator()(sample_type sample)
    {
        return sample * factor + *offsets++;
    }

    sample_type factor;
    const sample_type * offsets;
};



/* muladd helper, linear changing factor, constant offset */
template <typename sample_type>
struct muladd_helper_mul_l_add_c
{
    muladd_helper_mul_l_add_c(sample_type const & factor, sample_type const & factor_slope,
                              sample_type const & offset):
        factor(factor), factor_slope(factor_slope), offset(offset)
    {}

    sample_type operator()(sample_type sample)
    {
        sample_type ret = sample * factor + offset;
        factor += factor_slope;
        return ret;
    }

    sample_type factor, factor_slope, offset;
};

/* muladd helper, linear changing factor, linear changing offset */
template <typename sample_type>
struct muladd_helper_mul_l_add_l
{
    muladd_helper_mul_l_add_l(sample_type const & factor, sample_type const & factor_slope,
                              sample_type const & offset, sample_type const & offset_slope):
        factor(factor), factor_slope(factor_slope), offset(offset), offset_slope(offset_slope)
    {}

    sample_type operator()(sample_type sample)
    {
        sample_type ret = sample * factor + offset;
        offset += offset_slope;
        factor += factor_slope;
        return ret;
    }

    sample_type factor, factor_slope, offset, offset_slope;
};


/* muladd helper, linear changing factor, offset vector */
template <typename sample_type>
struct muladd_helper_mul_l_add_v
{
    muladd_helper_mul_l_add_v(sample_type const & factor, sample_type const & factor_slope,
                              sample_type * const & offsets):
        factor(factor), factor_slope(factor_slope), offsets( offsets )
    {}

    sample_type operator()(sample_type sample)
    {
        sample_type ret = sample * factor + *offsets++;
        factor += factor_slope;
        return ret;
    }

    sample_type factor, factor_slope;
    const sample_type * offsets;
};




/* muladd helper, factor vector, constant offset */
template <typename sample_type>
struct muladd_helper_mul_v_add_c
{
    muladd_helper_mul_v_add_c(sample_type * const & factors, sample_type const & offset):
        factors( factors ), offset(offset)
    {}

    sample_type operator()(sample_type sample)
    {
        return sample * *factors++ + offset;
    }

    const sample_type * factors;
    sample_type offset;
};

/* muladd helper, factor vector, linear changing offset */
template <typename sample_type>
struct muladd_helper_mul_v_add_l
{
    muladd_helper_mul_v_add_l(sample_type * const & factors, sample_type const & offset,
                              sample_type const & offset_slope):
        factors( factors ), offset(offset), offset_slope(offset_slope)
    {}

    sample_type operator()(sample_type sample)
    {
        sample_type ret = sample * *factors++ + offset;
        offset += offset_slope;
        return ret;
    }

    const sample_type * factors;
    sample_type offset, offset_slope;
};


/* muladd helper, factor vector, offset vector */
template <typename sample_type>
struct muladd_helper_mul_v_add_v
{
    /* offsets and factors should not point to the same memory region */
    muladd_helper_mul_v_add_v(sample_type * const & factors, sample_type * const & offsets):
        factors( factors ), offsets( offsets )
    {}

    sample_type operator()(sample_type sample)
    {
        return sample * *factors++ + *offsets++;
    }

    const sample_type * factors;
    const sample_type * offsets;
};

struct muladd_ugen:
    public Unit
{
    float mul, add;
};

/** defines ugen function wrappers and selector, should be used within a ugen class
 *
 *  CLASS_NAME: name of the ugen class, should be derived from muladd_ugen
 *  FUNCTION_NAME: a member function of the ugen class, should take a muladd helper as second argument
 *  INDEX: INDEX of the `mul' input. the `add' input is expected to be at INDEX+1
 *
 */
#define DEFINE_UGEN_FUNCTION_WRAPPER(CLASS_NAME, FUNCTION_NAME, INDEX)  \
    static void FUNCTION_NAME##_nop(CLASS_NAME * unit, int num_samples) \
    {                                                                   \
        muladd_helper_nop<float> ma;                                    \
        FUNCTION_NAME(unit, num_samples, ma);                           \
    }                                                                   \
                                                                        \
    static void FUNCTION_NAME##_mul_i(CLASS_NAME * unit, int num_samples) \
    {                                                                   \
        muladd_helper_mul_c<float> ma(IN0(INDEX));                      \
        FUNCTION_NAME(unit, num_samples, ma);                           \
    }                                                                   \
                                                                        \
    static void FUNCTION_NAME##_mul_k(CLASS_NAME * unit, int num_samples) \
    {                                                                   \
        float mul = IN0(INDEX);                                         \
        if (mul == unit->mul) {                                         \
            FUNCTION_NAME##_mul_i(unit, num_samples);                   \
        } else {                                                        \
            float slope = CALCSLOPE(mul, unit->mul);                    \
            muladd_helper_mul_l<float> ma(unit->mul, slope);            \
            unit->mul = mul;                                            \
            FUNCTION_NAME(unit, num_samples, ma);                       \
        }                                                               \
    }                                                                   \
                                                                        \
    static void FUNCTION_NAME##_mul_a(CLASS_NAME * unit, int num_samples) \
    {                                                                   \
        muladd_helper_mul_v<float> ma(IN(INDEX));                       \
        FUNCTION_NAME(unit, num_samples, ma);                           \
    }                                                                   \
                                                                        \
    static void FUNCTION_NAME##_add_i(CLASS_NAME * unit, int num_samples) \
    {                                                                   \
        muladd_helper_add_c<float> ma(IN0(INDEX+1));                    \
        FUNCTION_NAME(unit, num_samples, ma);                           \
    }                                                                   \
                                                                        \
    static void FUNCTION_NAME##_add_k(CLASS_NAME * unit, int num_samples) \
    {                                                                   \
        float add = IN0(INDEX+1);                                       \
        if (add == unit->add) {                                         \
            FUNCTION_NAME##_add_i(unit, num_samples);                   \
        } else {                                                        \
            float slope = CALCSLOPE(add, unit->add);                    \
            muladd_helper_add_l<float> ma(unit->add, slope);            \
            unit->add = add;                                            \
            FUNCTION_NAME(unit, num_samples, ma);                       \
        }                                                               \
    }                                                                   \
                                                                        \
    static void FUNCTION_NAME##_add_a(CLASS_NAME * unit, int num_samples) \
    {                                                                   \
        muladd_helper_add_v<float> ma(IN(INDEX+1));                     \
        FUNCTION_NAME(unit, num_samples, ma);                           \
    }                                                                   \
                                                                        \
                                                                        \
    static void FUNCTION_NAME##_mul_i_add_i(CLASS_NAME * unit, int num_samples) \
    {                                                                   \
        muladd_helper_mul_c_add_c<float> ma(IN0(INDEX), IN0(INDEX+1));  \
        FUNCTION_NAME(unit, num_samples, ma);                           \
    }                                                                   \
                                                                        \
    static void FUNCTION_NAME##_mul_i_add_k(CLASS_NAME * unit, int num_samples) \
    {                                                                   \
        float add = IN0(INDEX+1);                                       \
        if (add == unit->add) {                                         \
            FUNCTION_NAME##_mul_i_add_i(unit, num_samples);             \
        } else {                                                        \
            float slope = CALCSLOPE(add, unit->add);                    \
            muladd_helper_mul_c_add_l<float> ma(IN0(INDEX), unit->add, slope); \
            unit->add = add;                                            \
            FUNCTION_NAME(unit, num_samples, ma);                       \
        }                                                               \
    }                                                                   \
                                                                        \
    static void FUNCTION_NAME##_mul_k_add_i(CLASS_NAME * unit, int num_samples) \
    {                                                                   \
        float mul = IN0(INDEX);                                         \
        if (mul == unit->mul) {                                         \
            FUNCTION_NAME##_mul_i_add_i(unit, num_samples);             \
        } else {                                                        \
            float slope = CALCSLOPE(mul, unit->mul);                    \
            muladd_helper_mul_l_add_c<float> ma(unit->mul, slope, IN0(INDEX+1)); \
            unit->mul = mul;                                            \
            FUNCTION_NAME(unit, num_samples, ma);                       \
        }                                                               \
    }                                                                   \
                                                                        \
    static void FUNCTION_NAME##_mul_k_add_k(CLASS_NAME * unit, int num_samples) \
    {                                                                   \
        float mul = IN0(INDEX);                                         \
        float add = IN0(INDEX+1);                                       \
        if (mul == unit->mul) {                                         \
            FUNCTION_NAME##_mul_i_add_k(unit, num_samples);             \
        } else {                                                        \
            float mul_slope = CALCSLOPE(mul, unit->mul);                \
            if (add == unit->add) {                                     \
                muladd_helper_mul_l_add_c<float> ma(unit->mul, mul_slope, add); \
                unit->mul = mul;                                        \
                FUNCTION_NAME(unit, num_samples, ma);                   \
            }                                                           \
            else                                                        \
            {                                                           \
                float add_slope = CALCSLOPE(add, unit->add);            \
                muladd_helper_mul_l_add_l<float> ma(unit->mul, mul_slope, unit->add, add_slope); \
                unit->mul = mul;                                        \
                unit->add = add;                                        \
                FUNCTION_NAME(unit, num_samples, ma);                   \
            }                                                           \
        }                                                               \
    }                                                                   \
                                                                        \
    static void FUNCTION_NAME##_mul_i_add_a(CLASS_NAME * unit, int num_samples) \
    {                                                                   \
        muladd_helper_mul_c_add_v<float> ma(IN0(INDEX), IN(INDEX+1));   \
        FUNCTION_NAME(unit, num_samples, ma);                           \
    }                                                                   \
                                                                        \
    static void FUNCTION_NAME##_mul_a_add_i(CLASS_NAME * unit, int num_samples) \
    {                                                                   \
        muladd_helper_mul_v_add_c<float> ma(IN(INDEX), IN0(INDEX+1));   \
        FUNCTION_NAME(unit, num_samples, ma);                           \
    }                                                                   \
                                                                        \
    static void FUNCTION_NAME##_mul_a_add_k(CLASS_NAME * unit, int num_samples) \
    {                                                                   \
        float add = IN0(INDEX+1);                                       \
        if (add == unit->add) {                                         \
            FUNCTION_NAME##_mul_a_add_i(unit, num_samples);             \
        } else {                                                        \
            float slope = CALCSLOPE(add, unit->add);                    \
            muladd_helper_mul_v_add_l<float> ma(IN(INDEX), unit->add, slope); \
            unit->add = add;                                            \
            FUNCTION_NAME(unit, num_samples, ma);                       \
        }                                                               \
    }                                                                   \
                                                                        \
    static void FUNCTION_NAME##_mul_k_add_a(CLASS_NAME * unit, int num_samples) \
    {                                                                   \
        float mul = IN0(INDEX);                                         \
        if (mul == unit->mul) {                                         \
            FUNCTION_NAME##_mul_i_add_a(unit, num_samples);             \
        } else {                                                        \
            float slope = CALCSLOPE(mul, unit->mul);                    \
            muladd_helper_mul_l_add_v<float> ma(unit->mul, slope, IN(INDEX+1)); \
            unit->mul = mul;                                            \
            FUNCTION_NAME(unit, num_samples, ma);                       \
        }                                                               \
    }                                                                   \
                                                                        \
    static void FUNCTION_NAME##_mul_a_add_a(CLASS_NAME * unit, int num_samples) \
    {                                                                   \
        muladd_helper_mul_v_add_v<float> ma(IN(INDEX), IN(INDEX+1));    \
        FUNCTION_NAME(unit, num_samples, ma);                           \
    }                                                                   \
                                                                        \
    static UnitCalcFunc FUNCTION_NAME##_select(const CLASS_NAME * unit) \
    {                                                                   \
        switch(INRATE(INDEX))                                           \
        {                                                               \
        case calc_ScalarRate:                                           \
            switch(INRATE(INDEX+1))                                     \
            {                                                           \
            case calc_ScalarRate:                                       \
                if ((IN0(INDEX) == 1.f) && (IN0(INDEX+1) == 0.f))       \
                    return (UnitCalcFunc)(FUNCTION_NAME##_nop);         \
                if (IN0(INDEX) == 1.f)                                  \
                    return (UnitCalcFunc)(FUNCTION_NAME##_add_i);       \
                if (IN0(INDEX+1) == 0.f)                                \
                    return (UnitCalcFunc)(FUNCTION_NAME##_mul_i);       \
                return (UnitCalcFunc)(FUNCTION_NAME##_mul_i_add_i);     \
                                                                        \
            case calc_BufRate:                                          \
                if (IN0(INDEX) == 1.f)                                  \
                    return (UnitCalcFunc)(FUNCTION_NAME##_add_k);       \
                else                                                    \
                    return (UnitCalcFunc)(FUNCTION_NAME##_mul_i_add_k); \
                                                                        \
            case calc_FullRate:                                         \
                if (IN0(INDEX) == 1.f)                                  \
                    return (UnitCalcFunc)(FUNCTION_NAME##_add_a);       \
                else                                                    \
                    return (UnitCalcFunc)(FUNCTION_NAME##_mul_i_add_a); \
                                                                        \
            default:                                                    \
                assert(false);                                          \
            }                                                           \
                                                                        \
        case calc_BufRate:                                              \
            switch(INRATE(INDEX+1))                                     \
            {                                                           \
            case calc_ScalarRate:                                       \
                if (IN0(INDEX+1) == 0.f)                                \
                    return (UnitCalcFunc)(FUNCTION_NAME##_mul_k);       \
                else                                                    \
                    return (UnitCalcFunc)(FUNCTION_NAME##_mul_k_add_i); \
                                                                        \
            case calc_BufRate:                                          \
                return (UnitCalcFunc)(FUNCTION_NAME##_mul_k_add_k);     \
                                                                        \
            case calc_FullRate:                                         \
                return (UnitCalcFunc)(FUNCTION_NAME##_mul_k_add_a);     \
                                                                        \
            default:                                                    \
                assert(false);                                          \
            }                                                           \
                                                                        \
        case calc_FullRate:                                             \
            switch(INRATE(INDEX+1))                                     \
            {                                                           \
            case calc_ScalarRate:                                       \
                if (IN0(INDEX+1) == 0.f)                                \
                    return (UnitCalcFunc)(FUNCTION_NAME##_mul_a);       \
                else                                                    \
                    return (UnitCalcFunc)(FUNCTION_NAME##_mul_a_add_i); \
                                                                        \
            case calc_BufRate:                                          \
                return (UnitCalcFunc)(FUNCTION_NAME##_mul_a_add_k);     \
                                                                        \
            case calc_FullRate:                                         \
                return (UnitCalcFunc)(FUNCTION_NAME##_mul_a_add_a);     \
                                                                        \
            default:                                                    \
                assert(false);                                          \
            }                                                           \
                                                                        \
        default:                                                        \
            assert(false);                                              \
        }                                                               \
    }



#endif /* SC_MULADD_HELPERS_HPP */

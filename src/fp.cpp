#include <mcl/fp_base.hpp>

namespace mcl { namespace fp {

#ifdef USE_MONT_FP
FpGenerator *createFpGenerator()
{
	return new FpGenerator();
}

void destroyFpGenerator(FpGenerator* fg)
{
	delete fg;
}
#else
FpGenerator *createFpGenerator()
{
	return 0;
}

void destroyFpGenerator(FpGenerator*)
{
}
#endif

template<size_t bitN>
struct OpeFunc {
	typedef fp::Unit Unit;
	static const size_t N = (bitN + sizeof(Unit) * 8 - 1) / (sizeof(Unit) * 8);
	static inline void set_mpz_t(mpz_t& z, const Unit* p, int n = (int)N)
	{
		z->_mp_alloc = n;
		int i = n;
		while (i > 0 && p[i - 1] == 0) {
			i--;
		}
		z->_mp_size = i;
		z->_mp_d = (mp_limb_t*)const_cast<Unit*>(p);
	}
	static inline void set_zero(mpz_t& z, Unit *p, size_t n)
	{
		z->_mp_alloc = (int)n;
		z->_mp_size = 0;
		z->_mp_d = (mp_limb_t*)p;
	}
	static inline void clearC(Unit *x)
	{
		local::clearArray(x, 0, N);
	}
	static inline void copyC(Unit *y, const Unit *x)
	{
		local::copyArray(y, x, N);
	}
	static inline void addC(Unit *z, const Unit *x, const Unit *y, const Unit *p)
	{
		Unit ret[N + 2]; // not N + 1
		mpz_t mz, mx, my, mp;
		set_zero(mz, ret, N + 2);
		set_mpz_t(mx, x);
		set_mpz_t(my, y);
		set_mpz_t(mp, p);
		mpz_add(mz, mx, my);
		if (mpz_cmp(mz, mp) >= 0) {
			mpz_sub(mz, mz, mp);
		}
		local::toArray(z, N, mz);
	}
	static inline void subC(Unit *z, const Unit *x, const Unit *y, const Unit *p)
	{
		Unit ret[N + 1];
		mpz_t mz, mx, my;
		set_zero(mz, ret, N + 1);
		set_mpz_t(mx, x);
		set_mpz_t(my, y);
		mpz_sub(mz, mx, my);
		if (mpz_sgn(mz) < 0) {
			mpz_t mp;
			set_mpz_t(mp, p);
			mpz_add(mz, mz, mp);
		}
		local::toArray(z, N, mz);
	}
	static inline void mulPreC(Unit *z, const Unit *x, const Unit *y)
	{
		mpz_t mx, my, mz;
		set_zero(mz, z, N * 2);
		set_mpz_t(mx, x);
		set_mpz_t(my, y);
		mpz_mul(mz, mx, my);
		local::toArray(z, N * 2, mz);
	}
	// x[N * 2] -> y[N]
	static inline void modC(Unit *y, const Unit *x, const Unit *p)
	{
		mpz_t mx, my, mp;
		set_mpz_t(mx, x, N * 2);
		set_mpz_t(my, y);
		set_mpz_t(mp, p);
		mpz_mod(my, mx, mp);
		local::clearArray(y, my->_mp_size, N);
	}
	static inline void invOp(Unit *y, const Unit *x, const Op& op)
	{
		mpz_class my;
		mpz_t mx, mp;
		set_mpz_t(mx, x);
		set_mpz_t(mp, op.p);
		mpz_invert(my.get_mpz_t(), mx, mp);
		local::toArray(y, N, my.get_mpz_t());
	}
	static inline bool isZeroC(const Unit *x)
	{
		return local::isZeroArray(x, N);
	}
	static inline void negC(Unit *y, const Unit *x, const Unit *p)
	{
		if (isZeroC(x)) {
			if (x != y) clearC(y);
			return;
		}
		subC(y, p, x, p);
	}
};

#ifdef MCL_USE_LLVM
	#define SET_OP_LLVM(n) \
		op.addP = mcl_fp_add ## n ##S; \
		op.subP = mcl_fp_sub ## n ##S; \
		op.mulPreP = mcl_fp_mulPre ## n;
#else
	#define SET_OP_LLVM(n)
#endif

#define SET_OP(n) \
		op.N = n / UnitBitN; \
		op.isZero = OpeFunc<n>::isZeroC; \
		op.clear = OpeFunc<n>::clearC; \
		op.copy = OpeFunc<n>::copyC; \
		op.negP = OpeFunc<n>::negC; \
		op.invOp = OpeFunc<n>::invOp; \
		op.addP = OpeFunc<n>::addC; \
		op.subP = OpeFunc<n>::subC; \
		op.mulPreP = OpeFunc<n>::mulPreC; \
		op.modP = OpeFunc<n>::modC; \
		SET_OP_LLVM(n)

void setOp(mcl::fp::Op& op, const Unit* /*p*/, size_t bitLen)
{
	assert(sizeof(mp_limb_t) == sizeof(Unit));
	const size_t UnitBitN = sizeof(Unit) * 8;

	if (bitLen <= 128) {
		SET_OP(128)
	} else
#if CYBOZU_OS_BIT == 32
	if (bitLen <= 160) {
		SET_OP(160)
	} else
#endif
	if (bitLen <= 192) {
		SET_OP(192)
	} else
#if CYBOZU_OS_BIT == 32
	if (bitLen <= 224) {
		SET_OP(224)
	} else
#endif
	if (bitLen <= 256) {
		SET_OP(256)
	} else
	if (bitLen <= 384) {
		SET_OP(384)
	} else
#if CYBOZU_OS_BIT == 64
	if (bitLen <= 576) {
		SET_OP(576)
	}
#else
	if (bitLen <= 544) {
		SET_OP(544)
	}
#endif

#ifdef MCL_USE_LLVM
	if (op.mp == mpz_class("0xfffffffffffffffffffffffffffffffeffffffffffffffff")) {
		op.mul = &mcl_fp_mul_NIST_P192; // slower than MontFp192
	}
#endif
}

} } // mcl::fp

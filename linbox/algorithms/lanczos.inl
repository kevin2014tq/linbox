/* linbox/algorithms/lanczos.inl
 * Copyright (C) 2002 Bradford Hovinen
 *
 * Written by Bradford Hovinen <hovinen@cis.udel.edu>
 *
 * ------------------------------------
 *
 *
 * ========LICENCE========
 * This file is part of the library LinBox.
 *
 * LinBox is free software: you can redistribute it and/or modify
 * it under the terms of the  GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * ========LICENCE========
 *.
 */

#ifndef __LINBOX_lanczos_INL
#define __LINBOX_lanczos_INL

#include <vector>
#include <algorithm>

#include "linbox/blackbox/archetype.h"
#include "linbox/blackbox/diagonal.h"
#include "linbox/blackbox/transpose.h"
#include "linbox/util/debug.h"
#include "linbox/vector/vector-domain.h"

namespace LinBox
{

#ifdef DETAILED_TRACE

	template <class Field, class LVector>
	void traceReport (std::ostream &out, VectorDomain<Field> &VD, const char *text, size_t iter, const LVector &v)
	{
		out << text << " [" << iter << "]: ";
		VD.write (out, v) << std::endl;
	}

	template <class Field, class LVector>
	void traceReport (std::ostream &out, const Field &F, const char *text, size_t iter, const typename Field::Element &a)
	{
		out << text << " [" << iter << "]: ";
		F.write (out, a) << std::endl;
	}

#else

	template <class Field, class LVector>
	inline void traceReport (std::ostream &out, VectorDomain<Field> &VD, const char *text, size_t iter, const LVector &v)
	{}

	template <class Field, class LVector>
	void traceReport (std::ostream &out, const Field &F, const char *text, size_t iter, const typename Field::Element &a)
	{}

#endif

	template <class Field, class LVector>
	template <class Blackbox>
	LVector &LanczosSolver<Field, LVector>::solve (const Blackbox &A, LVector &x, const LVector &b)
	{
		linbox_check ((x.size () == A.coldim ()) &&
			      (b.size () == A.rowdim ()));

		commentator().start ("Solving linear system (Lanczos)", "LanczosSolver::solve");

		bool success = false;
		LVector d1, d2, b1, b2, bp, y, Ax, ATAx, ATb;

		VectorWrapper::ensureDim (_w[0], A.coldim ());
		VectorWrapper::ensureDim (_w[1], A.coldim ());
		VectorWrapper::ensureDim (_Aw, A.coldim ());

		Givaro::GeneralRingNonZeroRandIter<Field> real_ri (_randiter);
		RandomDenseStream<Field, LVector, Givaro::GeneralRingNonZeroRandIter<Field> > stream (field(), real_ri, A.coldim ());

		for (unsigned int i = 0; !success && i < _traits.trialsBeforeFailure; ++i) {
			std::ostream &report = commentator().report (Commentator::LEVEL_UNIMPORTANT, INTERNAL_DESCRIPTION);

			switch (_traits.preconditioner ) {
			case Preconditioner::None:
				success = iterate (A, x, b);
				break;

			case Preconditioner::Symmetrize:
				{
					VectorWrapper::ensureDim (bp, A.coldim ());

					Transpose<Blackbox> AT (&A);
					Compose<Transpose<Blackbox>, Blackbox> B (&AT, &A);

					AT.apply (bp, b);

					success = iterate (B, x, bp);

					break;
				}

			case Preconditioner::PartialDiagonal:
				{
					VectorWrapper::ensureDim (d1, A.coldim ());
					VectorWrapper::ensureDim (y, A.coldim ());

					stream >> d1;
					Diagonal<Field, typename VectorTraits<LVector>::VectorCategory> D (d1);
					Compose<Blackbox, Diagonal<Field, typename VectorTraits<LVector>::VectorCategory> > B (&A, &D);

					report << "Random D: ";
					_VD.write (report, d1) << std::endl;

					if ((success = iterate (B, y, b)))
						D.apply (x, y);

					break;
				}

			case Preconditioner::PartialDiagonalSymmetrize:
				{
					VectorWrapper::ensureDim (d1, A.rowdim ());
					VectorWrapper::ensureDim (b1, A.rowdim ());
					VectorWrapper::ensureDim (bp, A.coldim ());

					stream >> d1;
					Diagonal<Field, typename VectorTraits<LVector>::VectorCategory> D (d1);
					Transpose<Blackbox> AT (&A);
					Compose<Diagonal<Field, typename VectorTraits<LVector>::VectorCategory>, Blackbox> B1 (&D, &A);
					Compose<Transpose<Blackbox>, Compose<Diagonal<Field, typename VectorTraits<LVector>::VectorCategory>, Blackbox> > B (&AT, &B1);

					report << "Random D: ";
					_VD.write (report, d1) << std::endl;

					D.apply (b1, b);
					AT.apply (bp, b1);

					success = iterate (B, x, bp);

					break;
				}

			case Preconditioner::FullDiagonal:
				{
					VectorWrapper::ensureDim (d1, A.coldim ());
					VectorWrapper::ensureDim (d2, A.rowdim ());
					VectorWrapper::ensureDim (b1, A.rowdim ());
					VectorWrapper::ensureDim (b2, A.coldim ());
					VectorWrapper::ensureDim (bp, A.coldim ());
					VectorWrapper::ensureDim (y, A.coldim ());

					stream >> d1 >> d2;
					Diagonal<Field, typename VectorTraits<LVector>::VectorCategory> D1 (d1);
					Diagonal<Field, typename VectorTraits<LVector>::VectorCategory> D2 (d2);
					Transpose<Blackbox> AT (&A);

					Compose<Blackbox,
					Diagonal<Field, typename VectorTraits<LVector>::VectorCategory> > B1 (&A, &D1);

					Compose<Diagonal<Field, typename VectorTraits<LVector>::VectorCategory>,
					Compose<Blackbox,
					Diagonal<Field, typename VectorTraits<LVector>::VectorCategory> > > B2 (&D2, &B1);

					Compose<Transpose<Blackbox>,
					Compose<Diagonal<Field, typename VectorTraits<LVector>::VectorCategory>,
					Compose<Blackbox,
					Diagonal<Field, typename VectorTraits<LVector>::VectorCategory> > > > B3 (&AT, &B2);

					Compose<Diagonal<Field, typename VectorTraits<LVector>::VectorCategory>,
					Compose<Transpose<Blackbox>,
					Compose<Diagonal<Field, typename VectorTraits<LVector>::VectorCategory>,
					Compose<Blackbox,
					Diagonal<Field, typename VectorTraits<LVector>::VectorCategory> > > > > B (&D1, &B3);

					report << "Random D_1: ";
					_VD.write (report, d1) << std::endl;

					report << "Random D_2: ";
					_VD.write (report, d2) << std::endl;

					D2.apply (b1, b);
					AT.apply (b2, b1);
					D1.apply (bp, b2);

					if ((success = iterate (B, y, bp)))
						D1.apply (x, y);

					break;
				}

			default:
				throw PreconditionFailed (__func__, __LINE__,
							  "preconditioner should be None, Symmetrize, PartialDiagonal,"
							  "PartialDiagonalSymmetrize, or FullDiagonal");
			}

			if (success && _traits.checkResult) {
				VectorWrapper::ensureDim (Ax, A.rowdim ());

				if ((_traits.preconditioner == Preconditioner::Symmetrize) ||
				    (_traits.preconditioner == Preconditioner::PartialDiagonalSymmetrize) ||
				    (_traits.preconditioner == Preconditioner::FullDiagonal))
				{
					VectorWrapper::ensureDim (ATAx, A.coldim ());
					VectorWrapper::ensureDim (ATb, A.coldim ());

					commentator().start ("Checking whether A^T Ax = A^T b");

					A.apply (Ax, x);
					A.applyTranspose (ATAx, Ax);
					A.applyTranspose (ATb, b);

					if (_VD.areEqual (ATAx, ATb))
						commentator().stop ("passed");
					else {
						commentator().stop ("FAILED");
						success = false;
					}
				}
				else {
					commentator().start ("Checking whether Ax=b");

					A.apply (Ax, x);

					if (_VD.areEqual (Ax, b))
						commentator().stop ("passed");
					else {
						commentator().stop ("FAILED");
						success = false;
					}
				}
			}
		}

		if (success) {
			commentator().stop ("done", "Solve successful", "BlockLanczosSolver::solve");
			return x;
		}
		else {
			commentator().stop ("done", "Solve failed", "BlockLanczosSolver::solve");
			throw SolveFailed ();
		}
	}

	template <class Field, class LVector>
	template<class Blackbox>
	bool LanczosSolver<Field, LVector>::iterate (const Blackbox &A, LVector &x, const LVector &b)
	{
		commentator().start ("Lanczos iteration", "LanczosSolver::iterate", A.coldim ());

		// j is really a flip-flop: 0 means "even" and 1 means "odd". So "j" and
		// "j-2" are accessed with [j], while "j-1" and "j+1" are accessed via
		// [1-j]

		unsigned int j = 1, prods = 1, iter = 2;

		// N.B. For purposes of efficiency, I am purposefully making the
		// definitions of alpha and beta to be the *negatives* of what are given
		// in the Lambert thesis. This allows me to use stock vector AXPY
		// without any special modifications.

		typename Field::Element alpha, beta, delta[2], wb;

		// Zero out the vector _w[0]
		_VD.subin (_w[0], _w[0]);

		// Get a random vector _w[1]
		RandomDenseStream<Field, LVector> stream (field(), _randiter, A.coldim ());
		stream >> _w[1];

		std::ostream &report = commentator().report (Commentator::LEVEL_UNIMPORTANT, INTERNAL_DESCRIPTION);

		traceReport (report, _VD, "w", 1, _w[1]);

		A.apply (_Aw, _w[j]);                // Aw_j
		_VD.dot (delta[j], _w[j], _Aw);      // delta_j <- <w_j, Aw_j>

		if (field().isZero (delta[j])) {
			commentator().stop ("FAILED", "<w_1, Aw_1> = 0", "LanczosSolver::iterate");
			return false;
		}

		_VD.dot (alpha, _Aw, _Aw);           //   alpha <- -<Aw_j, Aw_j> / delta_j
		field().divin (alpha, delta[j]);
		field().negin (alpha);

		field().subin (beta, beta);               //    beta <- 0

		_VD.dot (wb, _w[j], b);              //       x <- <w_j, b> / delta_j w_j
		field().divin (wb, delta[j]);
		_VD.mul (x, _w[j], wb);

		while (!field().isZero (delta[j])) {
			commentator().progress ();

			report << "Total matrix-vector products so far: " << prods << std::endl;

			// 		traceReport (report, field(), "alpha", iter, alpha);
			// 		traceReport (report, field(), "beta", iter, alpha);
			traceReport (report, _VD, "w", iter - 1, _w[1 - j]);
			traceReport (report, _VD, "w", iter, _w[j]);

			_VD.mulin (_w[1 - j], beta);    //   w_j+1 <- Aw_j + alpha w_j + beta w_j-1
			_VD.axpyin (_w[1 - j], alpha, _w[j]);
			_VD.addin (_w[1 - j], _Aw);

			traceReport (report, _VD, "w", iter + 1, _w[1 - j]);
			traceReport (report, _VD, "Aw", iter, _Aw);

			j = 1 - j;                      //       j <- j + 1

			A.apply (_Aw, _w[j]);           // Aw_j

			_VD.dot (delta[j], _w[j], _Aw); // delta_j <- <w_j, Aw_j>

			// 		traceReport (report, field(), "delta", iter - 1, delta[1 - j]);
			// 		traceReport (report, field(), "delta", iter, delta[j]);

			if (!field().isZero (delta[j])) {
				_VD.dot (alpha, _Aw, _Aw);             // alpha <- -<Aw_j, Aw_j> / delta_j
				field().divin (alpha, delta[j]);
				field().negin (alpha);

				field().div (beta, delta[j], delta[1 - j]); //  beta <- -delta_j / delta_j-1
				field().negin (beta);

				_VD.dot (wb, _w[j], b);                //     x <- x + <w_j, b> / delta_j w_j
				field().divin (wb, delta[j]);
				_VD.axpyin (x, wb, _w[j]);
			}

			++prods;
			++iter;
		}

		commentator().indent (report);
		report << "Total matrix-vector products: " << prods << std::endl;

		commentator().stop ("done", "delta_j = 0", "LanczosSolver::iterate");
		return true;
	}

}  // namespace LinBox

#endif // __LINBOX_lanczos_INL

// Local Variables:
// mode: C++
// tab-width: 4
// indent-tabs-mode: nil
// c-basic-offset: 4
// End:
// vim:sts=4:sw=4:ts=4:et:sr:cino=>s,f0,{0,g0,(0,\:0,t0,+0,=s

/* Copyright (C)  LinBox
 * Written by Zhendong Wan
 *
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * ========LICENCE========
 */

#ifndef __LINBOX_smith_form_binary_H
#define __LINBOX_smith_form_binary_H

/*! @file algorithms/smith-form-binary.h
 *  Implementation of EGV and EGV+ algorithm
 */


#include "linbox/util/debug.h"
#include "linbox/algorithms/default.h"
#include "linbox/util/commentator.h"

namespace LinBox
{

	/** \brief Compute Smith form.
	 *
	 * This is an implementation of EGV and EGV+ algorithms
	 * See EGV (FOCS '00) and SW (ISSAC '04) papers.
	 */
	template<class _Ring, class _oneInvariantFactor, class _Rank>
	class SmithFormBinary {

	public:

		typedef _Ring Ring;

		typedef _oneInvariantFactor oneInvariantFactor;

		typedef _Rank Rank;

		typedef typename Ring::Element Integer;

	protected:

		oneInvariantFactor oif;
		Rank rank;
		Ring r;

	public:

		/** \brief constructor
		*/
		SmithFormBinary(const oneInvariantFactor& _oif =oneInvariantFactor(),
				const Rank& _rank =Rank(),
				const Ring& _r = Ring(),
				int _oifthreshold =DEFAULTOIFTHRESHOLD,
				int _lifthreshold =DEFAULTLIFTHRESHOLD) :
			oif(_oif),rank(_rank),r(_r)
		{

			oif.setThreshold(_oifthreshold);
			oif.getLastInvariantFactor().setThreshold( _lifthreshold);
		}

		void setOIFThreshold (int _oifthreshold =DEFAULTOIFTHRESHOLD)
		{
			oif.setThreshold(_oifthreshold);
		}

		void setLIFThreshold (int _lifthreshold =DEFAULTLIFTHRESHOLD)
		{
			oif.getLastInvariantFactor().setThreshold(_lifthreshold);
		}

		int getOIFThreshold() const
		{
			return oif.getThreshold();
		}

		int getLIFThreshold() const
		{
			return oif.getLastInvariantFactor().getlIFThreshold();
		}


		/** \brief compute the Smith Form of an integer matrix,
		 *  ignoring these factors of primes in PrimeL
		 */
		template<class IMatrix, class Vector, class VectorP>
		Vector&  smithForm(Vector& sf, const IMatrix& A, const VectorP& PrimeL)
		{

			// check if there are enough spaces in sf to store all invariant factors of A
			linbox_check(sf.size() >= (A.rowdim() <= A.coldim() ? A.rowdim() : A.coldim()));

			std::ostream& report = commentator().report (Commentator::LEVEL_IMPORTANT, INTERNAL_DESCRIPTION);

			typename Vector::iterator p;


			long Ar = rank.rank(A);


			report << "Rank = " << Ar <<'\n';


			// set k-th invariant factor to zero for all k > Ar
			for (p = sf.begin() + Ar; p!= sf.end(); ++p)
				r.assign(*p,r.zero);


			// A is a zero matrix
			if (Ar == 0) {

				report << "Smith Form:[ ";

				for (p = sf.begin(); p != sf.end(); ++ p) {

					r. write (report, *p);

					report << ' ';
				}

				report <<"]\n";


				return sf;
			}


			// compute first invariant factor of A
			firstInvariantFactor(sf[0], A, PrimeL);

			report << "First Invariant Factor = ";

			r. write (report, sf[0]);

			report << '\n' << std::flush;

			// if rank(A) == 1
			if (Ar == 1) {

				report << "Smith Form:[ ";

				for (p = sf.begin(); p != sf.end(); ++ p) {

					r. write (report, *p);

					report << ' ';
				}

				report <<"]\n" << std::flush;

				return sf;
			}


			oif.oneInvariantFactor(sf[(size_t)Ar - 1], A, (int)Ar, PrimeL);

			report << "Biggest invariant factor = ";

			r. write (report, sf[(size_t)Ar - 1]);

			report << '\n' << std::flush;

			// binary search smith form
			smithFormBinarySearch (sf, A, 1, (int)Ar, PrimeL);

			report << "Smith Form:[ ";

			for (p = sf.begin(); p != sf.end(); ++ p) {

				r. write (report, *p);

				report << ' ';
			}

			report << "]\n" << std::flush;

			return sf;
		}


		/** \brief compute the Smith Form of an integer matrix
		*/
		template<class IMatrix, class Vector>
		Vector&  smithFormBinary(Vector& sf, const IMatrix& A)
		{

			std::vector<Integer> empty_v;

			smithForm (sf, A, empty_v);

			return sf;

		}

	protected:

		/** \brief compute the 1st invariant factor, = GCD (all element in A),
		 *  missing these factors of primes in PrimeL
		 */
		template<class IMatrix, class Vector>
		Integer& firstInvariantFactor(Integer& fif, const IMatrix& A, const Vector& PrimeL)
		{

			r.assign(fif,r.zero);

			typename IMatrix::ConstIterator A_p;

			for (A_p = A.Begin(); A_p != A.End(); ++ A_p) {

				if (!r.isZero(*A_p)) {
					r.gcd(fif, fif, *A_p);

					// if tmp == 1, break
					if (r.isOne(fif)) return fif;
				}
			}


			if (r.isZero(fif)) return fif;

			Integer p, quo, rem;

			typename Vector::const_iterator Prime_p;

			// filter out primes in PRIME from lif
			for ( Prime_p = PrimeL.begin(); Prime_p != PrimeL.end(); ++ Prime_p) {

				r.init (p, *Prime_p);

				do {
					r.quoRem(quo,rem,fif,p);

					if (r.isZero( rem )) r.assign(fif,quo);
					else break;
				}
				while (true);

			}

			return fif;

		}


		/** \brief Binary search invariant factors between i and j, missing those factors in PrimeL
		 *  suppose sf[(size_t)i - 1], sf [j - 1] are ith and jth invariant factor of A
		 *  i <= j
		 */

		template<class IMatrix, class Vector, class VectorP>
		Vector& smithFormBinarySearch (Vector& sf, const IMatrix& A, int i, int j, const VectorP& PrimeL)
		{


			std::ostream& report = commentator().report (Commentator::LEVEL_IMPORTANT, INTERNAL_DESCRIPTION);

			report << "Binary Search invariant factors [" << i << ", "<< j << "]\n " << std::flush;

			typename Vector::iterator p;

			// if no invariant factor between i and j
			if (j <= i + 1) return sf;

			// if i-th invariant factor == j-th invariant factor
			if (r.areEqual(sf[(size_t)i - 1], sf[(size_t)j - 1])) {
				for (p = sf.begin() + i; p != sf.begin() + (j -1); ++ p)
					r.assign (*p, sf[(size_t)i-1]);
				return sf;
			}

			int mid = (i + j) / 2;

			report << "Start to compute " << mid << "-th invariant factor:\n" << std::flush;


			oif.oneInvariantFactor (sf[(size_t)mid - 1], A, mid, PrimeL);


			report << mid <<"-th invairant factor of A = " ;
			r.write (report, sf[(size_t)mid -1]);
			report << "\n" << std::flush;


			// recursively binary search all k-invariant factors, where i <= k <= mid
			smithFormBinarySearch (sf, A, i, mid, PrimeL);

			// recurseively binary search all k-invariant factors, where mid <= k < j
			smithFormBinarySearch (sf, A, mid, j, PrimeL);

			return sf;
		}


	public:
		/** \brief compute the Smith Form of an integer matrix,
		 *  ignoring these factors of primes in PrimeL
		 *  Using backward search descibed by B. D. Saunders.
		 */
		template<class IMatrix, class Vector, class VectorP>
		Vector&  smithFormBackward(Vector& sf, const IMatrix& A, const VectorP& PrimeL)
		{

			// check if there are enough spaces in sf to store all invariant factors of A
			linbox_check(sf.size() >= (A.rowdim() <= A.coldim() ? A.rowdim() : A.coldim()));

			std::ostream& report = commentator().report (Commentator::LEVEL_IMPORTANT, INTERNAL_DESCRIPTION);

			typename Vector::iterator p;


			long Ar = rank.rank(A);


			report << "Rank = " << Ar <<'\n';


			// set k-th invariant factor to zero for all k > Ar
			for (p = sf.begin() + Ar; p!= sf.end(); ++p)
				r.assign(*p,r.zero);


			// A is a zero matrix
			if (Ar == 0) {

				report << "Smith Form:[ ";

				for (p = sf.begin(); p != sf.end(); ++ p) {

					r. write (report, *p);

					report << ' ';
				}

				report <<"]\n";


				return sf;
			}


			// compute first invariant factor of A
			firstInvariantFactor(sf[0], A, PrimeL);

			report << "First Invariant Factor = ";

			r. write (report, sf[0]);

			report << '\n' << std::flush;

			// if rank(A) == 1
			if (Ar == 1) {

				report << "Smith Form:[ ";

				for (p = sf.begin(); p != sf.end(); ++ p) {

					r. write (report, *p);

					report << ' ';
				}

				report <<"]\n" << std::flush;

				return sf;
			}


			oif.oneInvariantFactor(sf[(size_t)Ar - 1], A, Ar, PrimeL);

			report << "Biggest invariant factor = ";

			r. write (report, sf[(size_t)Ar - 1]);

			report << '\n' << std::flush;

			// binary search smith form
			smithFormBinarySearchBackward (sf, A, 1, Ar, 1, PrimeL);

			report << "Smith Form:[ ";

			for (p = sf.begin(); p != sf.end(); ++ p) {

				r. write (report, *p);

				report << ' ';
			}

			report << "]\n" << std::flush;

			return sf;
		}


		/** \brief compute the Smith Form of an integer matrix
		 * Using backward binary search.
		 */
		template<class IMatrix, class Vector>
		Vector&  smithFormBackward(Vector& sf, const IMatrix& A)
		{

			std::vector<Integer> empty_v;

			smithFormBackward (sf, A, empty_v);

			return sf;

		}

	protected:

		/** \brief Binary search invariant factors between i and j, missing those factors in PrimeL
		 *  suppose sf[(size_t)i - 1], sf [j - 1] are ith and jth invariant factor of A
		 *  i <= j
		 */

		template<class IMatrix, class Vector, class VectorP>
		Vector& smithFormBinarySearchBackward (Vector& sf, const IMatrix& A, int i, int j, int depth, const VectorP& PrimeL)
		{


			std::ostream& report = commentator().report (Commentator::LEVEL_IMPORTANT, INTERNAL_DESCRIPTION);

			report << "Binary Search invariant factors [" << i << ", "<< j << "]\n " << std::flush;

			typename Vector::iterator p;

			// if no invariant factor between i and j
			if (j <= i + 1) return sf;

			// if i-th invariant factor == j-th invariant factor
			if (r.areEqual(sf[(size_t)i - 1], sf[(size_t)j - 1])) {
				for (p = sf.begin() + i; p != sf.begin() + (j -1); ++ p)
					r.assign (*p, sf[(size_t)i-1]);
				return sf;
			}

			int mid = std::max (j -depth,  (i + j) / 2);

			report << "Start to compute " << mid << "-th invariant factor:\n" << std::flush;


			oif.oneInvariantFactor (sf[(size_t)mid - 1], A, mid, PrimeL);


			report << mid <<"-th invairant factor of A = " ;
			r.write (report, sf[(size_t)mid -1]);
			report << "\n" << std::flush;


			// recursively binary search all k-invariant factors, where i <= k <= mid

			if (r. areEqual (sf[(size_t)mid-1], sf[(size_t)j-1]))
				smithFormBinarySearchBackward (sf, A, i, mid, 2 * depth, PrimeL);
			else
				smithFormBinarySearchBackward (sf, A, i, mid, depth, PrimeL);

			// recurseively binary search all k-invariant factors, where mid <= k < j
			smithFormBinarySearchBackward (sf, A, mid, j, depth, PrimeL);

			return sf;
		}
	};

}

#endif  //__LINBOX_smith_form_binary_H

// Local Variables:
// mode: C++
// tab-width: 4
// indent-tabs-mode: nil
// c-basic-offset: 4
// End:
// vim:sts=4:sw=4:ts=4:et:sr:cino=>s,f0,{0,g0,(0,\:0,t0,+0,=s

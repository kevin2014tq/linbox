/* linbox/algorithms/wiedemann.h
 * Copyright (C) 2002 Zhendong Wan
 * Copyright (C) 2002 Bradford Hovinen
 *
 * Written by Zhendong Wan <wan@mail.eecis.udel.edu>,
 *            Bradford Hovinen <hovinen@cis.udel.edu>
 *
 * ----------------------------------------------------
 * 2003-02-05  Bradford Hovinen  <bghovinen@math.uwaterloo.ca>
 *
 * Ripped out all the exception code. Exceptions decided one day to
 * just stop working on my compiler, and they were controversal
 * anyway. Now all the solve functions return a status. There are most
 * likely still bugs in this code, though.
 * ----------------------------------------------------
 * 2002-10-02  Bradford Hovinen  <bghovinen@math.uwaterloo.ca>
 *
 * Refactoring:
 * Put everything inside a WiedemannSolver class, with the following
 * interface:
 *    solveNonsingular - Solve a nonsingular system
 *    solveSingular - Solve a general singular system
 *    findRandomSolution - Find a random solution to a singular preconditioned
 *                         problem
 *    findNullSpaceElement - Find an element of the right nullspace
 *    certifyInconsistency - Find a certificate of inconsistency for a
 *                           linear system
 *    precondition - Form a preconditioner and return it
 * ------------------------------------
 * 2002-08-09  Bradford Hovinen  <hovinen@cis.udel.edu>
 *
 * Move the Wiedemann stuff over to this file
 *
 * Create a singular and nonsingular version that is a bit intelligent about
 * which one to use in different circumstances
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

#ifndef __LINBOX_wiedemann_H
#define __LINBOX_wiedemann_H

/*! @file algorithms/wiedemann.h
 * @ingroup algorithms
 * @brief minpoly computation and Wiedeman solvers.
 */

#include <vector>
#include <algorithm>

#include "linbox/blackbox/archetype.h"
#include "linbox/blackbox/squarize.h"
#include "linbox/matrix/sparse-matrix.h"
#include "linbox/util/debug.h"
#include "linbox/vector/vector-domain.h"
#include "linbox/solutions/methods.h"

#include "linbox/algorithms/blackbox-container.h"
#include "linbox/algorithms/blackbox-container-symmetric.h"

// massey recurring sequence solver
#include "linbox/algorithms/massey-domain.h"

namespace LinBox
{


	template<class Polynomial, class Blackbox>
	Polynomial &minpoly (Polynomial& P,
			     const Blackbox& A,
			     RingCategories::ModularTag tag,
			     const Method::Wiedemann& M = Method::Wiedemann ())
	{
		typedef typename Blackbox::Field Field;
		typename Field::RandIter i (A.field());
		size_t            deg;

		commentator().start ("Wiedemann Minimal polynomial", "minpoly");

		if (A.coldim() != A.rowdim()) {
			commentator().report(Commentator::LEVEL_IMPORTANT, INTERNAL_DESCRIPTION) << "Virtually squarize matrix" << std::endl;

			Squarize<Blackbox> B(&A);
			BlackboxContainer<Field, Squarize<Blackbox> > TF (&B, A.field(), i);
			MasseyDomain< Field, BlackboxContainer<Field, Squarize<Blackbox> > > WD (&TF, M.earlyTerminationThreshold);

			WD.minpoly (P, deg);
		}
		else if (M.shapeFlags == Shape::Symmetric) {
			typedef BlackboxContainerSymmetric<Field, Blackbox> BBContainerSym;
			BBContainerSym TF (&A, A.field(), i);
			MasseyDomain< Field, BBContainerSym > WD (&TF, M.earlyTerminationThreshold);

			WD.minpoly (P, deg);
		}
		else {
			typedef BlackboxContainer<Field, Blackbox> BBContainer;
			BBContainer TF (&A, A.field(), i);
			MasseyDomain< Field, BBContainer > WD (&TF, M.earlyTerminationThreshold);

			WD.minpoly (P, deg);
#ifdef INCLUDE_TIMING
			commentator().report (Commentator::LEVEL_IMPORTANT, TIMING_MEASURE)
			<< "Time required for applies:      " << TF.applyTime () << std::endl;
			commentator().report (Commentator::LEVEL_IMPORTANT, TIMING_MEASURE)
			<< "Time required for dot products: " << TF.dotTime () << std::endl;
			commentator().report (Commentator::LEVEL_IMPORTANT, TIMING_MEASURE)
			<< "Time required for discrepency:  " << WD.discrepencyTime () << std::endl;
			commentator().report (Commentator::LEVEL_IMPORTANT, TIMING_MEASURE)
			<< "Time required for LSR fix:      " << WD.fixTime () << std::endl;
#endif // INCLUDE_TIMING
		}

//             std::cerr << "P: " << P << std::endl;
//             std::cerr << "WD deg: " << deg << std::endl;
        if (!deg) {
                // zero sequence, matrix minpoly is X
            P.resize(2);
            A.field().assign(P[0],A.field().zero);
            A.field().assign(P[1],A.field().one);
        }

		commentator().stop ("done", NULL, "minpoly");

		return P;
	}
}

#ifndef LINBOX_EXTENSION_DEGREE_MAX
#define LINBOX_EXTENSION_DEGREE_MAX 19
#endif

#include <givaro/extension.h>
#include <givaro/gfq.h>

#include "linbox/matrix/sparse-matrix.h"
#include "linbox/ring/modular.h"
#include "linbox/algorithms/matrix-hom.h"
#include "linbox/field/map.h"

namespace LinBox
{
	// The minpoly with BlackBox Method
	template<class Polynomial, class Blackbox>
	Polynomial &minpoly (
			     Polynomial         &P,
			     const Blackbox                            &A,
			     const RingCategories::ModularTag          &tag,
			     const Method::WiedemannExtension& M)
	{
		typedef typename Blackbox::Field Field;
		const Field& F = A.field();
		integer a,c; F.cardinality(a); F.characteristic(c);
		if (a != c) {
			uint64_t extend = (uint64_t)FF_EXPONENT_MAX(a,(integer)LINBOX_EXTENSION_DEGREE_MAX);
			if (extend > 1) {
				commentator().report (Commentator::LEVEL_ALWAYS,INTERNAL_WARNING) << "Extension of degree " << extend << std::endl;
				Givaro::Extension<Field> EF( F, extend);

				typedef typename Blackbox::template rebind< Givaro::Extension<Field>  >::other FBlackbox;

				FBlackbox Ap(A, EF);

				BlasVector< Givaro::Extension<Field> > eP(EF);
				minpoly(eP, Ap, tag, Method::Wiedemann(M));

				return PreMap<Field, Givaro::Extension<Field> >(F,EF)(P, eP);
			}
			else
				return minpoly(P, A, tag, Method::Wiedemann(M));
		}
		else {
			uint64_t extend = (uint64_t)FF_EXPONENT_MAX(c,(integer)LINBOX_EXTENSION_DEGREE_MAX);
			if (extend > 1) {
				commentator().report (Commentator::LEVEL_ALWAYS,INTERNAL_WARNING) << "Word size extension : " << extend << std::endl;
				typedef Givaro::GFqDom<int64_t> Fld;
				Fld EF( (uint64_t)c, extend);
				typedef typename Blackbox::template rebind< Fld >::other FBlackbox;
				FBlackbox Ap(A, EF);
				BlasVector< Fld > eP(EF);
				minpoly(eP, Ap, tag, Method::Wiedemann(M));
				return PreMap<Field, Fld >(F,EF)(P, eP);

			}
			else
				return minpoly(P, A, tag, Method::Wiedemann(M));
		}
	}
}

/*namespace LinBox
{
	// The minpoly with BlackBox Method
	template<class Polynomial, class Blackbox>
	Polynomial &minpoly (
			     Polynomial         &P,
			     const Blackbox                            &A,
			     const RingCategories::ModularTag          &tag,
			     const Method::WiedemannExtension& M)
	{
		commentator().report (Commentator::LEVEL_ALWAYS,INTERNAL_WARNING) << " WARNING, no extension available, returning only a factor of the minpoly\n";
		return minpoly(P, A, tag, Method::Wiedemann (M));
	}
}*/

namespace LinBox
{
	/** \brief Linear system solvers based on Wiedemann's method.
	 *
	 * This class encapsulates all of the functionality for linear system
	 * solving with Wiedemann's algorithm. It includes the random solution and
	 * random nullspace element of Kaltofen and Saunders (1991), as well as the
	 * certificate of inconsistency of Giesbrecht, Lobo, and Saunders (1998).
	 */
	template <class Field>
	class WiedemannSolver {
	public:

		/// { OK, FAILED, SINGULAR, INCONSISTENT, BAD_PRECONDITIONER }
		enum ReturnStatus {
			OK, FAILED, SINGULAR, INCONSISTENT, BAD_PRECONDITIONER
		};

		/*! Constructor.
		 *
		 * @param F Field over which to operate
		 * @param traits @ref SolverTraits  structure describing user
		 *               options for the solver
		 */
		WiedemannSolver (const Field &F, const Method::Wiedemann &traits) :
			_traits (traits), _field (&F), _randiter (F), _VD (F)
		{}

		/*! Constructor with a random iterator.
		 *
		 * @param F Field over which to operate
		 * @param traits @ref SolverTraits  structure describing user
		 *               options for the solver
		 * @param r Random iterator to use for randomization
		 */
		WiedemannSolver (const Field &F,
				 const Method::Wiedemann &traits,
				 typename Field::RandIter r) :
			_traits (traits), _field (&F), _randiter (r), _VD (F)
		{}

		/// \ingroup algorithms
		/// \defgroup Solvers Solvers

		//@{

		/*! Solve a system Ax=b, giving a random solution if the system is
		 * singular and consistent, and a certificate of inconsistency (if
		 * specified in traits parameter at construction time) otherwise.
		 *
		 * @param A Black box of linear system
		 * @param x Vector in which to store solution
		 * @param b Right-hand side of system
		 * @param u Vector in which to store certificate of inconsistency
		 * @return Reference to solution vector
		 */
		template<class Blackbox, class Vector>
		ReturnStatus solve (const Blackbox&A, Vector &x, const Vector &b, Vector &u);

		/*! Solve a nonsingular system Ax=b.
		 *
		 * This is a "Las Vegas" method, which makes use of randomization. It
		 * attempts to certify that the system solution is correct. It will only
		 * make one attempt to solve the system before giving up.
		 *
		 * @param A Black box of linear system
		 * @param x Vector in which to store solution
		 * @param b Right-hand side of system
		 * @param useRandIter true if solveNonsingular should use a random
		 *                    iterator for the Krylov sequence computation;
		 *                    false if it should use the right-hand side
		 * @return Reference to solution vector
		 */
		template<class Blackbox, class Vector>
		ReturnStatus solveNonsingular (const Blackbox&A,
					       Vector &x,
					       const Vector &b,
					       bool useRandIter = false);

		/*! Solve a general singular linear system.
		 *
		 * @param A Black box of linear system
		 * @param x Vector in which to store solution
		 * @param b Right-hand side of system
		 * @param u Vector into which certificate of inconsistency will be stored
		 * @param r Rank of A
		 * @return Return status
		 */
		template<class Blackbox, class Vector>
		ReturnStatus solveSingular (const Blackbox&A,
					    Vector &x,
					    const Vector &b,
					    Vector &u,
					    size_t r);

		/*! Get a random solution to a singular system Ax=b of rank r with
		 * generic rank profile.
		 *
		 * @param A Black box of linear system
		 * @param x Vector in which to store solution
		 * @param b Right-hand side of system
		 * @param r Rank of A
		 * @param P Left preconditioner (NULL if none needed)
		 * @param Q Right preconditioner (NULL if none needed)
		 * @return Return status
		 */
		template<class Blackbox, class Vector, class Prec1, class Prec2>
		ReturnStatus findRandomSolution (const Blackbox          &A,
						 Vector                  &x,
						 const Vector            &b,
						 size_t                   r,
						 const Prec1             *P,
						 const Prec2             *Q);

		/*! Get a random element of the right nullspace of A of rank r.
		 *
		 * @param x Vector in which to store nullspace element
		 * @param A Black box of which to find nullspace element
		 */
		template<class Blackbox, class Vector>
		ReturnStatus findNullspaceElement (Vector                &x,
						   const Blackbox        &A, const size_t r);

		/*! Get a certificate \p u that the given system \f$Ax=b\f$ is
		 * of rank r and inconsistent, if one can be found.
		 *
		 * @param u Vector in which to store certificate
		 * @param A Blackbox for the linear system
		 * @param b Right-hand side for the linear system
		 * @return \p true if a certificate can be found in one iteration; \p u
		 *         is filled in with that certificate; and \p false otherwise
		 */
		template<class Blackbox, class Vector>
		bool certifyInconsistency (Vector                          &u,
					   const Blackbox                  &A,
					   const Vector                    &b, const size_t r);

		//@}

		inline const Field & field() const { return *_field; }

	private:

		// Make an m x m lambda-sparse matrix, c.f. Mulders (2000)
		SparseMatrix<Field> *makeLambdaSparseMatrix (size_t m);

		Method::Wiedemann                      _traits;
		const Field                         *_field;
		typename Field::RandIter             _randiter;
		VectorDomain<Field>                  _VD;
	};

}

#include "linbox/algorithms/wiedemann.inl"

#endif //  __LINBOX_wiedemann_H

// Local Variables:
// mode: C++
// tab-width: 4
// indent-tabs-mode: nil
// c-basic-offset: 4
// End:
// vim:sts=4:sw=4:ts=4:et:sr:cino=>s,f0,{0,g0,(0,\:0,t0,+0,=s

/* linbox/algorithms/localsmith.h
 * Copyright(C) LinBox
 *
 * Written by David Saunders
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

#ifndef __LINBOX_smith_form_local_H
#define __LINBOX_smith_form_local_H

#include <vector>
#include <list>
//#include <algorithm>

// #include "linbox/field/multimod-field.h"
#include "linbox/matrix/dense-matrix.h"

namespace LinBox
{
    /**
      \brief Smith normal form (invariant factors) of a matrix over a local ring.

      The matrix must be a BlasMatrix over a LocalPID.
      A localPID has the standard ring/field arithmetic functions plus gcdin().
     */
    template <class LocalPID>
    class SmithFormLocal{

    public:
        typedef typename LocalPID::Element Elt;

        template<class Matrix>
        std::list<Elt>& operator()(std::list<Elt>& L, Matrix& A, const LocalPID& R)
        {
            Elt d; R.assign(d,R.one);
            return smithStep(L, d, A, R);
        }

        template<class Matrix>
        std::list<Elt>& smithStep(std::list<Elt>& L, Elt& d, Matrix& A, const LocalPID& R)
        {
            if ( A.rowdim() == 0 || A.coldim() == 0 ) return L;
        	
            Elt g; R.assign(g, R.zero);
            typename Matrix::RowIterator p;
            typename Matrix::Row::iterator q, r;
            for ( p = A.rowBegin(); p != A.rowEnd(); ++p) {
                for (q = p->begin(); q != p->end(); ++q) {
                    R.gcdin(g, *q);
                    
                    if ( R.isUnit(g) ) {
                        //R.divin(g, g); break;
                        R.assign(g, R.one); break;
                    }
                }

                if ( R.isUnit(g) ) break;
            }

            if ( R.isZero(g) ) {
                L.insert(L.end(), (A.rowdim() < A.coldim()) ? A.rowdim() : A.coldim(), g);
                return L;
            }

            if ( p != A.rowEnd() ) {
            	
                // g is a unit and,
                // because this is a local ring, value at which this first happened
                // also is a unit.
                if ( p != A.rowBegin() )
                    swap_ranges(A.rowBegin()->begin(), A.rowBegin()->end(), p->begin());
                
                if ( q != p->begin() )
                    swap_ranges(A.colBegin()->begin(), A.colBegin()->end(),
                            (A.colBegin() +(int) (q - p->begin()))->begin());

                // eliminate step - crude and for dense only - fix later
                // Want to use a block method or "left looking" elimination.
                Elt f; R.inv(f, *(A.rowBegin()->begin() ) );
                R.negin(f);
                
                // normalize first row to -1, ...
                for ( q = A.rowBegin()->begin() /*+ 1*/; q != A.rowBegin()->end(); ++q)
                    R.mulin(*q, f);
                
                //
                // eliminate in subsequent rows
                size_t i = 0, j = 0;
                for (p = A.rowBegin() + 1; p != A.rowEnd(); ++p) {
                	i++;
                    for (q = p->begin() + 1, r = A.rowBegin()->begin() + 1, f = *(p -> begin()); q != p->end(); ++q, ++r) {
                    	j++;
                    	
                        R.axpyin( *q, f, *r );
                    }
                }

				// submatrix!
                BlasSubmatrix<BlasMatrix<LocalPID> > Ap(A, 1, 1, A.rowdim() - 1, A.coldim() - 1);
                L.push_back(d);
                return smithStep(L, d, Ap, R);
            } else {
                typename Matrix::Iterator p_it;
                for (p_it = A.Begin(); p_it != A.End(); ++p_it) {
                   R.divin(*p_it, g);
                }
				typename LocalPID::Element x; R.neg(x, g);
				R.gcdin(g,x);
                return smithStep(L, R.mulin(d, g), A, R);
            }
        }
    }; // end SmithFormLocal
} // end LinBox

#include "linbox/algorithms/smith-form-local2.inl"
#endif // __LINBOX_smith_form_local_H

// Local Variables:
// mode: C++
// tab-width: 4
// indent-tabs-mode: nil
// c-basic-offset: 4
// End:
// vim:sts=4:sw=4:ts=4:et:sr:cino=>s,f0,{0,g0,(0,\:0,t0,+0,=s

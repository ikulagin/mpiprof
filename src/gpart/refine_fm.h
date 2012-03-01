/*
 * refine_fm.h: Fiduccia-Mattheyses heuristic [1, 2] for refine bisections.
 *
 * [1] C.M. Fiduccia and R. M. Mattheyses. A linear-time heuristic for improving
 *     network partitions // Proc. of Conference "Design Automation",
 *     1982. – P. 175-181.
 * [2] G. Karypis and V. Kumar. A fast and high quality multilevel scheme for
 *     partitioning irregular graphs // SIAM Journal on Scientific Computing,
 *     1999, Vol. 20, No. 1, P.359-392.
 *
 * (C) 2007-2010 Mikhail Kurnosov <mkurnosov@gmail.com>
 */

#ifndef REFINE_FM_H
#define REFINE_FM_H

#include "gpart.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * gpart_refine_bisection_fm: Refine bisection by Fiduccia-Mattheyses heuristic.
 *                            Return edge-cut in edgecut.
 *                            Return 0 on success and 1 otherwise.
 */
int gpart_refine_bisection_fm(csrgraph_t *g, int *bisection, int *partsizes,
		                      edgecut_t *edgecut);

/* gpart_balance_bisection_fm: Balance bisection by FM heuristic. */
int gpart_balance_bisection_fm(csrgraph_t *g, int *bisection, int nmoves,
		                       int moveto);

#ifdef __cplusplus
}
#endif

#endif /* REFINE_FM_H */


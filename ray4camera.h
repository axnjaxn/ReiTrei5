#ifndef _BPJ_REITREI4_CAMERA_H
#define _BPJ_REITREI4_CAMERA_H

#include "ray4object.h"
#include <cmath>

class Ray4Camera : public Ray4Object {
public:
  REAL cellw, cellh;
  int pxw, pxh;

  virtual Ray4Intersection intersectsUnit(const Vect4& O, const Vect4& D) { }

  Ray4Camera() {setScreen(100, 100, 1, 1, PI / 2);}

  void setScreen(int pxw, int pxh, REAL xrange, REAL yrange, REAL fov) {
    this->pxw = pxw;
    this->pxh = pxh;
    cellw = xrange * 2 * tan(fov / 2) / pxw;
    cellh = yrange * 2 * tan(fov / 2) / pxh;
  }
  inline Vect4 getOrigin() const {return transformPoint(M, Vect4());}
  inline Vect4 getDirection(REAL r, REAL c) const {return transformDirection(M, Vect4((c  - pxw / 2) * cellw, -(r - pxh / 2) * cellh, 1).unit()).unit();}
};

#endif

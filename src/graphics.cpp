#pragma GCC optimize ("O3")
/*
 * REminiscence - Flashback interpreter
 * Copyright (C) 2005-2019 Gregory Montoir (cyx@users.sourceforge.net)
 */
extern "C" {
#include <string.h>
extern unsigned char *hwram_screen;
}
#include "graphics.h"

#define VIDEO_PITCH 240

void Graphics::setLayer(uint8_t *layer, int pitch) {
	_layer = layer;
//	_layerPitch = pitch;
}

void Graphics::setClippingRect(int16_t rx, int16_t ry, int16_t rw, int16_t rh) {
//	debug(DBG_VIDEO, "Graphics::setClippingRect(%d, %d, %d, %d)", rx, ry, rw, rh);
	_crx = rx;
	_cry = ry;
	_crw = rw;
	_crh = rh;
}

void Graphics::drawPoint(uint8_t color, const Point *pt) {
//	debug(DBG_VIDEO, "Graphics::drawPoint() col=0x%X x=%d, y=%d", color, pt->x, pt->y);
	if (pt->x >= 0 && pt->x < _crw && pt->y >= 0 && pt->y < _crh) {
		*(_layer + (pt->y + _cry) * VIDEO_PITCH + pt->x + _crx) = color;
	}
}

void Graphics::drawLine(uint8_t color, const Point *pt1, const Point *pt2) {
    int16_t x = pt1->x + _crx, y = pt1->y + _cry;
    int16_t dx = pt2->x - pt1->x, dy = pt2->y - pt1->y;
    int16_t sx = dx < 0 ? -1 : 1, sy = dy < 0 ? -1 : 1;
    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;

    int16_t err = (dx > dy ? dx : -dy) / 2, e2;
    uint8_t *layerBase = _layer + _cry * VIDEO_PITCH + _crx;

    for (;;) {
        if ((uint16_t)x < _crw && (uint16_t)y < _crh) {
            layerBase[y * VIDEO_PITCH + x] = color;
        }
        if (x == pt2->x + _crx && y == pt2->y + _cry) break;
        e2 = err;
        if (e2 > -dx) { err -= dy; x += sx; }
        if (e2 < dy) { err += dx; y += sy; }
    }
}

/*
void Graphics::drawLine(uint8_t color, const Point *pt1, const Point *pt2) {
//	debug(DBG_VIDEO, "Graphics::drawLine()");
	int16_t dxincr1 = 1;
	int16_t dyincr1 = 1;
	int16_t dx = pt2->x - pt1->x;
	if (dx < 0) {
		dxincr1 = -1;
		dx = -dx;
	}
	int16_t dy = pt2->y - pt1->y;
	if (dy < 0) {
		dyincr1 = -1;
		dy = -dy;
	}
	int16_t dxincr2, dyincr2, delta1, delta2;
	if (dx < dy) {
		dxincr2 = 0;
		dyincr2 = 1;
		delta1 = dx;
		delta2 = dy;
		if (dyincr1 < 0) {
			dyincr2 = -1;
		}
	} else {
		dxincr2 = 1;
		dyincr2 = 0;
		delta1 = dy;
		delta2 = dx;
		if (dxincr1 < 0) {
			dxincr2 = -1;
		}
	}
	Point pt;
	pt.x = pt1->x;
	pt.y = pt1->y;
	int16_t octincr1 = delta1 * 2 - delta2 * 2;
	int16_t octincr2 = delta1 * 2;
	int16_t oct = delta1 * 2 - delta2;
	if (delta2 >= 0) {
//		drawPoint(color, &pt);
		if (pt.x >= 0 && pt.x < _crw && pt.y >= 0 && pt.y < _crh) {
			*(_layer + (pt.y + _cry) * VIDEO_PITCH + pt.x + _crx) = color;
		}
		while (--delta2 >= 0) {
			if (oct >= 0) {
				pt.x += dxincr1;
				pt.y += dyincr1;
				oct += octincr1;
			} else {
				pt.x += dxincr2;
				pt.y += dyincr2;
				oct += octincr2;
			}
//			drawPoint(color, &pt);
			if (pt.x >= 0 && pt.x < _crw && pt.y >= 0 && pt.y < _crh) {
				*(_layer + (pt.y + _cry) * VIDEO_PITCH + pt.x + _crx) = color;
			}
		}
	}
}
*/
void Graphics::addEllipseRadius(int16_t y, int16_t x1, int16_t x2) {
//	debug(DBG_VIDEO, "Graphics::addEllipseRadius()");
	if (y >= 0 && y <= _crh) {
		y = (y - _areaPoints[0]) * 2;
		if (x1 < 0) {
			x1 = 0;
		}
		if (x2 >= _crw) {
			x2 = _crw - 1;
		}
		_areaPoints[y + 1] = x1;
		_areaPoints[y + 2] = x2;
	}
}

void Graphics::drawEllipse(uint8_t color, bool hasAlpha, const Point *pt, int16_t rx, int16_t ry) {
//if(hasAlpha)
//	emu_printf("Graphics::drawEllipse() %d color %d\n",hasAlpha, color);
	bool flag = false;
	int16_t y = pt->y - ry;
	if (y < 0) {
		y = 0;
	}
	if (y < _crh) {
		if (pt->y + ry >= 0) {
			_areaPoints[0] = y;
			int32_t dy = 0;
			int32_t rxsq  = rx * rx;
			int32_t rxsq2 = rx * rx * 2;
			int32_t rxsq4 = rx * rx * 4;
			int32_t rysq  = ry * ry;
			int32_t rysq2 = ry * ry * 2;
			int32_t rysq4 = ry * ry * 4;

			int32_t dx = 0;
			int32_t b = rx * ((rysq2 & 0xFFFF) + (rysq2 >> 16));
			int32_t a = 2 * b;

			int32_t ny1, ny2, nx1, nx2;
			ny1 = ny2 = rysq4 / 2 - a + rxsq;
			nx1 = nx2 = rxsq2 - b + rysq;

			while (ny2 < 0) {
				int16_t x2 = pt->x + rx;
				int16_t x1 = pt->x - rx;
				int16_t by = pt->y + dy;
				int16_t ty = pt->y - dy;
				if (x1 != x2) {
					addEllipseRadius(by, x1, x2);
					if (ty < by) {
						addEllipseRadius(ty, x1, x2);
					}
				}
				dy += 1;
				dx += rxsq4;
				nx1 = dx;
				if (nx2 < 0) {
					nx2 += nx1 + rxsq2;
					ny2 += nx1;
				} else {
					--rx;
					a -= rysq4;
					ny1 = a;
					nx2 += nx1 + rxsq2 - ny1;
					ny2 += nx1 + rysq2 - ny1;
				}
			}

			while (rx >= 0) {
				bool flag2 = false;
				int16_t x2 = pt->x + rx;
				int16_t x1 = pt->x - rx;
				int16_t by = pt->y + dy;
				int16_t ty = pt->y - dy;
				if (!flag && x1 != x2) {
					flag2 = true;
					addEllipseRadius(by, x1, x2);
					if (ty < by) {
						addEllipseRadius(ty, x1, x2);
					}
				}
				if (flag2) {
					flag = true;
				}
				--rx;
				a -= rysq4;
				nx1 = a;
				if (ny2 < 0) {
					++dy;
					flag = false;
					dx += rxsq4;
					ny2 += dx - nx1 + rysq2;
					ny1 = dx - nx1 + rysq2;
				} else {
					ny2 += rysq2 - nx1;
					ny1 = rysq2 - nx1;
				}
			}
			if (flag) {
				++dy;
			}

			while (dy <= ry) {
				int16_t ty = pt->y - dy;
				int16_t by = pt->y + dy;
				if (ty < by) {
					addEllipseRadius(ty, pt->x, pt->x);
				}
				addEllipseRadius(by, pt->x, pt->x);
				++dy;
			}
			y = pt->y + ry + 1;
			if (y > _crh) {
				y = _crh;
			}
			y = (y - _areaPoints[0]) * 2;
			_areaPoints[y + 1] = -1;
			fillArea(color, hasAlpha);
		}
	}
}

void Graphics::fillArea(uint8_t color, bool hasAlpha) {
    int16_t *pts = _areaPoints;
    uint8_t *dst = _layer + (*pts++ + _cry) * VIDEO_PITCH + _crx;
    int16_t x1 = *pts++;
    if (x1 < 0) return;

    if (!hasAlpha || color <= 7) {
        do {
            int16_t x2 = *pts++;
            if (x2 > _crw - 1) x2 = _crw - 1;
            if (x1 <= x2) {
                uint8_t *curDst = dst + x1;
                memset(curDst, color, x2 - x1 + 1);
            }
            dst += VIDEO_PITCH;
            x1 = *pts++;
        } while (x1 >= 0);
    } else {
        uint8_t *aux1 = hwram_screen + (VIDEO_PITCH * 128) + (*(_areaPoints) + _cry) * (VIDEO_PITCH / 2) + _crx / 2;
        do {
            int16_t x2 = *pts++;
            if (x2 > _crw - 1) x2 = _crw - 1;
            if (x1 <= x2) {
                uint8_t *curDst = dst + x1, *end = curDst + (x2 - x1 + 1);
                uint8_t *aux = aux1 + x1 / 2;
                int x = x1;
                uint8_t mask = color & ~7;
                while (curDst < end) {
                    uint8_t val = *curDst | mask;
                    if (val != 8) {
                        *curDst++ = val;
                    } else {
                        uint8_t nibble = (x & 1) ? *aux : (*aux >> 4);
                        *curDst++ = (nibble & 7) + 8;
                    }
                    if (x & 1) ++aux;
                    ++x;
                }
            }
            dst += VIDEO_PITCH;
            aux1 += VIDEO_PITCH / 2;
            x1 = *pts++;
        } while (x1 >= 0);
    }
}

void Graphics::drawSegment(uint8_t color, bool hasAlpha, int16_t ys, const Point *pts, uint8_t numPts) {
//if(hasAlpha)
//	emu_printf("Graphics::drawSegment() %d color %d\n",hasAlpha, color);
    int16_t xmin = pts[0].x, xmax = xmin;
    for (uint8_t i = 1; i < numPts; ++i) {
        int16_t x = pts[i].x;
        if (x < xmin) xmin = x;
        else if (x > xmax) xmax = x;
    }
    if (xmin < 0) xmin = 0;
    if (xmax >= _crw) xmax = _crw - 1;
    if (xmin <= xmax) {
        _areaPoints[0] = ys;
        _areaPoints[1] = xmin;
        _areaPoints[2] = xmax;
        _areaPoints[3] = -1;
        fillArea(color, hasAlpha);
    }
}
/*
void Graphics::drawPolygonOutline(uint8_t color, const Point *pts, uint8_t numPts) {
//	debug(DBG_VIDEO, "Graphics::drawPolygonOutline()");
	assert(numPts >= 2);
	int i;
	for (i = 0; i < numPts - 1; ++i) {
		drawLine(color, &pts[i], &pts[i + 1]);
	}
	drawLine(color, &pts[i], &pts[0]);
}*/
void Graphics::drawPolygonOutline(uint8_t color, const Point *pts, uint8_t numPts) {
//	debug(DBG_VIDEO, "Graphics::drawPolygonOutline()");
    if (numPts < 2) return;
    const Point *p1 = pts;
    const Point *p2 = pts + 1;
    for (uint8_t i = 0; i < numPts; ++i) {
        drawLine(color, p1, p2);
        p1 = p2;
        p2 = (i == numPts - 2) ? pts : p2 + 1;
    }
}

static int32_t calcPolyStep(int16_t dx, int16_t dy, bool zeroFraction) {
//    if (dx == 0 || dy == 0) return 0;
    if (dy == 0) return 0;
	int32_t a = dx << 8;
	if ((a >> 16) < dy) {
		return ((int16_t)(a / dy)) << 8;
	} else {
		a = ((a >> 8) / dy);
		return zeroFraction ? a & 0xFFFF0000 : a << 16;
	}
}

static void drawPolygonHelper1(int32_t &x, int16_t &y, int32_t &step, int16_t *&pts, int16_t *&start) {
	bool first = true;
	x = pts[0];
	y = pts[1];
	int16_t dy, dx;
	do {
		if (first) {
			first = false;
		} else {
			x = *pts;
		}
		--pts;
		dy = *pts - y;
		--pts;
		dx = *pts - x;
	} while (dy <= 0 && start < pts);
	x <<= 16;
	if (dy > 0) {
//		step = calcPolyStep1(dx, dy);
		step = calcPolyStep(dx, dy, 1);
	}
}

static void drawPolygonHelper2(int32_t &x, int16_t &y, int32_t &step, int16_t *&pts, int16_t *&start) {
	bool first = true;
	x = *start++;
	y = *start++;
	int16_t dy, dx;
	do {
		if (first) {
			first = false;
		} else {
			x = *start;
			start += 2;
		}
		dy = start[1] - y;
		dx = start[0] - x;
	} while (dy <= 0 && start < pts);
	x <<= 16;
	if (dy > 0) {
//		step = calcPolyStep2(dx, dy);
		step = calcPolyStep(dx, dy, 0);
	}
}

void Graphics::drawPolygon(uint8_t color, bool hasAlpha, const Point *pts, uint8_t numPts) {
//	if(hasAlpha)
//	emu_printf("Graphics::drawPolygon(%d,%d,x%d,y%d,%d)\n",color,hasAlpha,pts->x,pts->y,numPts);
//	assert(numPts * 4 < 0x100);
	if(numPts * 4 >= 0x100)
	{
//		emu_printf("Graphics::drawPolygon(%d,%d,x%d,y%d,%d)\n",color,hasAlpha,pts->x,pts->y,numPts);
		return;
	}

	int16_t *apts1 = &_areaPoints[AREA_POINTS_SIZE];
	int16_t *apts2 = &_areaPoints[AREA_POINTS_SIZE + numPts * 2];

	int16_t xmin, xmax, ymin, ymax;
	xmin = xmax = pts[0].x;
	ymin = ymax = pts[0].y;

	int16_t *spts = apts1;
	*apts1++ = *apts2++ = pts[0].x;
	*apts1++ = *apts2++ = pts[0].y;

	for (int p = 1; p < numPts; ++p) {
		int16_t x = pts[p].x;
		int16_t y = pts[p].y;
		if (ymin > y) {
			ymin = y;
			spts = apts1;
		}
		if (ymax < y) {
			ymax = y;
		}
		*apts1++ = *apts2++ = x;
		*apts1++ = *apts2++ = y;

		if (xmin > x) {
			xmin = x;
		}
		if (xmax < x) {
			xmax = x;
		}
	}
	int16_t *rpts = _areaPoints;
	if (xmax < 0 || xmin >= _crw || ymax < 0 || ymin >= _crh) {
		return;
	}
	if (numPts == 2) {
		drawLine(color, &pts[0], &pts[1]);
		return;
	}
	if (ymax == ymin) {
		drawSegment(color, hasAlpha, ymax, pts, numPts);
		return;
	}
	int16_t x, dx, y, dy;
	int32_t a, b, d, f;
	int32_t xstep1 = 0;
	int32_t xstep2 = 0;

	apts1 = &spts[numPts * 2];
	xmax = _crw - 1;
	ymax = _crh - 1;
	const int32_t l1 = 65536;
	const int32_t l2 = -65536;
	if (ymin < 0) {
		int16_t x0, y0;
		do {
			--apts1;
			y0 = *apts1;
			--apts1;
			x0 = *apts1;
		} while (y0 < 0);
		x = apts1[2];
		y = apts1[3];
		dy = y0 - y;
		dx = x0 - x;
		xstep1 = (dy << 16) | dx;
		assert(dy != 0);
		a = y * dx / dy;
		b = (x - a) << 16;
//		d = xstep1 = calcPolyStep1(dx, dy);
		d = xstep1 = calcPolyStep(dx, dy, 1);
		if (d < 0) {
			d = -d;
		}
		if (d < l1) {
			d = l2;
		}
		d /= 2;
		b -= d;

		do {
			x0 = *spts++;
			y0 = *spts++;
		} while (*(spts + 1) < 0);
		dy = spts[1] - y0;
		dx = spts[0] - x0;
		xstep2 = (dy << 16) | dx;
		assert(dy != 0);
		a = y0 * dx / dy;
		f = (x0 - a) << 16;
//		d = xstep2 = calcPolyStep2(dx, dy);
		d = xstep2 = calcPolyStep(dx, dy, 0);
		if (d < 0) {
			d = -d;
		}
		if (d < l1) {
			d = l1;
		}
		d /= 2;
		f += d;
		ymin = 0;
		*rpts++ = 0;
		goto gfx_startLine;
	}

	*rpts++ = ymin;

gfx_startNewLine:
	drawPolygonHelper2(f, ymin, xstep2, apts1, spts);
	if (spts >= apts1) {
		b = apts1[0] << 16;
		dy = apts1[1];
		if (dy <= ymax) goto gfx_endLine;
		goto gfx_fillArea;
	}
	drawPolygonHelper1(b, ymin, xstep1, apts1, spts);
	d = xstep1;
	if (d < 0) {
		if (d >= l2) {
			d = l1;
		}
		d /= 2;
		b += d;
	}
	d = xstep2;
	if (d >= 0) {
		if (d <= l1) {
			d = l1;
		}
		d /= 2;
		f += d;
	}
	d = b;
	if (d < 0) {
		d = 0;
	}
	x = f >> 16;
	if (x > xmax) {
		x = xmax;
	}
	*rpts++ = d >> 16;
	*rpts++ = x;
	++ymin;
	d = xstep1;
	if (d >= 0) {
		if (d <= l1) {
			d = l1;
		}
		d /= 2;
	}
	b += d;
	d = xstep2;
	if (d < 0) {
		if (d >= l2) {
			d = l1;
		}
		d /= 2;
	}
	f += d;

gfx_startLine:
	while (1) {
		dy = apts1[1];
		if (spts >= apts1) {
			break;
		} else if (dy > spts[1]) {
			dy = spts[1];
			if (dy > ymax) {
				goto gfx_drawPolygonEnd;
			}
			dy -= ymin;
			if (dy > 0) {
				--dy;
				do {
					a = b;
					if (a < 0) {
						a = 0;
					}
					x = f >> 16;
					if (x > xmax) {
						x = xmax;
					}
					*rpts++ = a >> 16;
					*rpts++ = x;
					b += xstep1;
					f += xstep2;
					--dy;
				} while (dy >= 0);
			}
			drawPolygonHelper2(f, ymin, xstep2, apts1, spts);
			d = xstep2;
			if (d >= 0) {
				if (d <= l1) {
					d = l1;
				}
				d /= 2;
				f += d;
			} else {
				d = b;
				if (d < 0) {
					d = 0;
				}
				x = f >> 16;
				if (x > xmax) {
					x = xmax;
				}
				*rpts++ = d >> 16;
				*rpts++ = x;
				++ymin;
				d = xstep2;
				if (d >= l2) {
					d = l1;
				}
				d /= 2;
				f += d;
				b += xstep1;
			}
		} else if (dy == spts[1]) {
			if (dy > ymax) goto gfx_drawPolygonEnd;
			dy -= ymin;
			if (dy > 0) {
				--dy;
				do {
					a = b;
					if (a < 0) {
						a = 0;
					}
					x = f >> 16;
					if (x > xmax) {
						x = xmax;
					}
					*rpts++ = a >> 16;
					*rpts++ = x;
					b += xstep1;
					f += xstep2;
					--dy;
				} while (dy >= 0);
			}
			goto gfx_startNewLine;
		} else if (dy > ymax) {
			goto gfx_drawPolygonEnd;
		} else {
			dy -= ymin;
			if (dy > 0) {
				--dy;
				do {
					a = b;
					if (a < 0) {
						a = 0;
					}
					x = f >> 16;
					if (x > xmax) {
						x = xmax;
					}
					*rpts++ = a >> 16;
					*rpts++ = x;
					b += xstep1;
					f += xstep2;
					--dy;
				} while (dy >= 0);
			}
			drawPolygonHelper1(b, ymin, xstep1, apts1, spts);
			d = xstep1;
			if (d < 0) {
				if (d >= l2) {
					d = l1;
				}
				d /= 2;
				b += d;
			} else {
				d = b;
				if (d < 0) {
					d = 0;
				}
				x = f >> 16;
				if (x > xmax) {
					x = xmax;
				}
				*rpts++ = d >> 16;
				*rpts++ = x;
				++ymin;
				d = xstep1;
				if (d <= l1) {
					d = l1;
				}
				d /= 2;
				b += d;
				f += xstep2;
			}
		}
	}

	if (dy > ymax) goto gfx_drawPolygonEnd;
	dy -= ymin;
	if (dy < 0) goto gfx_fillArea;
	if (dy > 0) {
		--dy;
		do {
			a = b;
			if (a < 0) {
				a = 0;
			}
			x = f >> 16;
			if (x > xmax) {
				x = xmax;
			}
			*rpts++ = a >> 16;
			*rpts++ = x;
			b += xstep1;
			f += xstep2;
			--dy;
		} while (dy >= 0);
	}

	b = f = (apts1[0] << 16) | apts1[1];

gfx_endLine:
	d = xstep1;
	if (d >= 0) {
		if (d >= l1) {
			d /= 2;
			b -= d;
		}
	}
	d = xstep2;
	if (d < 0) {
		d /= 2;
		f -= d;
	}
	a = b;
	if (a < 0) {
		a = 0;
	}
	x = f >> 16;
	if (x > xmax) {
		x = xmax;
	}
	*rpts++ = a >> 16;
	*rpts++ = x;
	goto gfx_fillArea;

gfx_drawPolygonEnd:
	dy = ymax - ymin;
	if (dy >= 0) {
		do {
			a = b;
			if (a < 0) {
				a = 0;
			}
			x = f >> 16;
			if (x > xmax) {
				x = xmax;
			}
			*rpts++ = a >> 16;
			*rpts++ = x;
			b += xstep1;
			f += xstep2;
			--dy;
		} while (dy >= 0);
	}

gfx_fillArea:
	*rpts++ = -1;
	fillArea(color, hasAlpha);
}

//static int16_t _pointsQueue[8192];

void Graphics::floodFill(uint8_t color, const Point *pts, uint8_t numPts) {
	int16_t *_pointsQueue = (int16_t *)hwram_screen+16384;
//	assert(numPts >= 3);
	if(numPts < 3)
	{
		emu_printf("Graphics::floodFill\n");
		return;
	}
	int xmin, xmax;
	xmin = xmax = pts[0].x;
	int ymin, ymax;
	ymin = ymax = pts[0].y;
	for (int i = 0; i < numPts; ++i) {
		drawLine(color, &pts[i], &pts[(i == numPts - 1) ? 0 : (i + 1)]);
		if (xmin > pts[i].x) {
			xmin = pts[i].x;
		}
		if (xmax < pts[i].x) {
			xmax = pts[i].x;
		}
		if (ymin > pts[i].y) {
			ymin = pts[i].y;
		}
		if (ymax < pts[i].y) {
			ymax = pts[i].y;
		}
	}
	const uint32_t size = (xmax - xmin + 1) * 2 * (ymax - ymin + 1) * 2;
	if (size > 8192) {
		return;
	}
	int start_x = pts[0].x + 1;
	int start_y = pts[0].y + 1;
	while (start_x <= xmax && start_y <= ymax) {
		if (_layer[(start_y + _cry) * VIDEO_PITCH + (start_x + _crx)] != color) {
			uint32_t r = 0;
			uint32_t w = 0;
			_pointsQueue[w++] = start_x + _crx;
			_pointsQueue[w++] = start_y + _cry;
			xmin += _crx;
			xmax += _crx;
			ymin += _cry;
			ymax += _cry;
			_layer[(start_y + _cry) * VIDEO_PITCH + (start_x + _crx)] = color;
			while (r < w) {
				const int x = _pointsQueue[r++];
				const int y = _pointsQueue[r++];
				static const int8_t d[] = { -1, 0, 1, 0, 0, -1, 0, 1 };
				for (int i = 0; i < 8; i += 2) {
					const int x1 = x + d[i];
					const int y1 = y + d[i + 1];
					if (x1 >= xmin && x1 <= xmax && y1 >= ymin && y1 <= ymax && _layer[y1 * VIDEO_PITCH + x1] != color) {
						_layer[y1 * VIDEO_PITCH + x1] = color;
						_pointsQueue[w++] = x1;
						_pointsQueue[w++] = y1;
					}
				}
			}
			break;
		}
		++start_x;
		++start_y;
	}
}

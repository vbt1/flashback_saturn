
/*
 * REminiscence - Flashback interpreter
 * Copyright (C) 2005-2019 Gregory Montoir (cyx@users.sourceforge.net)
 */

#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__

#include "intern.h"

struct Graphics {
	static const int AREA_POINTS_SIZE = 256 * 2; // maxY * sizeof(Point) / sizeof(int16_t)
	uint8_t *_layer;
//	int _layerPitch;
	int16_t _areaPoints[AREA_POINTS_SIZE * 2];
	int16_t _crx, _cry, _crw, _crh;
	void setLayer(uint8_t *layer, int pitch);
	void setClippingRect(int16_t vx, int16_t vy, int16_t vw, int16_t vh);	
	void drawPoint(uint8 color, const Point *pt);
	void drawLine(uint8 color, const Point *pt1, const Point *pt2);
	void addEllipseRadius(int16 y, int16 x1, int16 x2);
	void drawEllipse(uint8 color, bool hasAlpha, const Point *pt, int16 rx, int16 ry);
	void fillArea(uint8 color, bool hasAlpha);
	void drawSegment(uint8 color, bool hasAlpha, int16 ys, const Point *pts, uint8 numPts);
	void drawPolygonOutline(uint8 color, const Point *pts, uint8 numPts);
	void drawPolygon(uint8 color, bool hasAlpha, const Point *pts, uint8 numPts);
};

#endif // __GRAPHICS_H__

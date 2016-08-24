/**
*    Copyright (C) 2015 LIESMARS, Wuhan University.
*    Financially supported by Wuda Geoinfamatics Co. ,Ltd.
*    Author:  Xiang Longgang, Wang Dehao , Shao Xiaotian
*
*    This program is free software: you can redistribute it and/or  modify
*    it under the terms of the GNU Affero General Public License, version 3,
*    as published by the Free Software Foundation.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma  once

#include <vector>
#include <algorithm>

using namespace std;

namespace geo_relationship{

	struct POINT
	{
		double x;
		double y;
		POINT(double a = 0, double b = 0) { x = a; y = b; } //constructor 

	};
	struct LINESEG
	{
		POINT s;
		POINT e;
		LINESEG(POINT a, POINT b) { s = a; e = b; }
	};


	double multiply(POINT sp, POINT ep, POINT op)
	{
		return ((sp.x - op.x) * (ep.y - op.y) - (ep.x - op.x) * (sp.y - op.y));
	}

	bool intersect(LINESEG u, LINESEG v)
	{
		return ((max(u.s.x, u.e.x) >= min(v.s.x, v.e.x)) && 
			(max(v.s.x, v.e.x) >= min(u.s.x, u.e.x)) &&
			(max(u.s.y, u.e.y) >= min(v.s.y, v.e.y)) &&
			(max(v.s.y, v.e.y) >= min(u.s.y, u.e.y)) &&
			(multiply(v.s, u.e, u.s) * multiply(u.e, v.e, u.s) >= 0) && 
			(multiply(u.s, v.e, v.s) * multiply(v.e, u.e, v.s) >= 0));
	}

	int Tangle(vector<POINT> t1, vector<POINT> t2)
	{
		int num = 0;
		for (int i = 0; i < t1.size() - 1; i++)
		{
			for (int j = 0; j < t2.size() - 1; j++)
			{
				POINT a(t1[i].x, t1[i].y);
				POINT b(t1[i + 1].x, t1[i + 1].y);
				POINT c(t2[j].x, t2[j].y);
				POINT d(t2[j + 1].x, t2[j + 1].y);

				LINESEG l1(a, b);
				LINESEG l2(c, d);

				if (intersect(l1, l2))
				{
					num++;
				}
			}

		}
		return num;
	}

	bool IsIn(POINT p, vector<POINT> t)
	{
		bool isIn = false;
		POINT g;
		g.x = p.x; g.y = 0;
		vector<POINT> data;
		data.push_back(g); data.push_back(p);
		if (Tangle(data, t) % 2 != 0)
		{
			isIn = true;
		}


		return isIn;

	}

	bool IsContantedBy(vector<POINT> t1, vector<POINT> t2)
	{
		bool isok = true;
		for (int i = 0; i < t1.size(); i++)
		{
			if (IsIn(t1[i], t2) == false)
			{
				isok = false;
			}
		}
		return isok;
	}



}
#include "simulation.h"

#ifdef __cplusplus
class slDrawCommand;

#include <list>

class slDrawCommandList {
	public:
		slDrawCommandList(slWorld *w) {
			_limit = 0;
			_drawingPolygon = 0;
			w->drawings.push_back(this);
		}

		~slDrawCommandList();

		void setCommandLimit(int n) { _limit = n; };
		int getCommandCount() { return _commands.size(); }

		void addCommand(slDrawCommand *command);
		void draw(slCamera *c);
		void clear();

	protected:
		std::list<slDrawCommand*> _commands;
		bool _drawingPolygon;
		unsigned int _limit;

		friend class slDrawEndPolygon;
		friend class slDrawCommandPoint;
		friend class slDrawCommandLine;
};

class slDrawCommand {
	public:
		virtual void execute(slDrawCommandList &list) = 0;
};

class slDrawCommandPoint : slDrawCommand {
	public:
		slDrawCommandPoint(slVector *p) : _point(*p) {}

		void execute(slDrawCommandList &list) {
			if(!list._drawingPolygon) {
				glBegin(GL_POLYGON);
				// glBegin(GL_TRIANGLE_FAN);
				list._drawingPolygon = 1;
			}

			glVertex3f(_point.x, _point.y, _point.z);
		}

	private:
		slVector _point;
};

class slDrawSetLineStyle : slDrawCommand {
	public:
		slDrawSetLineStyle(unsigned int s) : _style(s) {}

		void execute(slDrawCommandList &list) {
			glLineStipple(1, _style);
		}

	private:
		unsigned int _style;
};

class slDrawSetLineWidth : slDrawCommand {
	public:
		slDrawSetLineWidth(double w) : _width(w) {}

		void execute(slDrawCommandList &list) {
			glLineWidth(_width);
		}

	private:
		double _width;
};

class slDrawEndPolygon : slDrawCommand {
	public:
		void execute(slDrawCommandList &list) {
			if(list._drawingPolygon) {
				glEnd();
				list._drawingPolygon = 0;
			}

		}
};

class slDrawCommandLine : slDrawCommand {
	private:
		slVector _start, _end;

	public:
		slDrawCommandLine(slVector *s, slVector *e) : _start(*s), _end(*e) {}

		void execute(slDrawCommandList &list) {
			if(list._drawingPolygon) {
				list._drawingPolygon = 0;
				glEnd();
			}

			glBegin(GL_LINES);
			glVertex3f(_start.x, _start.y, _start.z);
			glVertex3f(_end.x, _end.y, _end.z);
			glEnd();
		}
};

class slDrawCommandColor : slDrawCommand {
	public:
		slDrawCommandColor(slVector *c, double a) : _color(*c), _alpha(a) {}

		void execute(slDrawCommandList &list) {
			glColor4f(_color.x, _color.y, _color.z, _alpha);
		}

	private:
		slVector _color;
		double _alpha;
};
#endif
/**
 * glfractal.h
 */

#ifndef GLFRACTAL_H
#define GLFRACTAL_H

#include <QtOpenGL/QGLWidget>
#include <QMouseEvent>

#include "kernel.h"

class breveGLWidget : public QGLWidget
{
  Q_OBJECT
public:
  breveGLWidget( QWidget* parent, const QGLWidget *share, Qt::WFlags f);
  ~breveGLWidget();
  
  void setEngine(brEngine *e) { 
		_engine = e; 

		if(_engine) {
			slInitGL(_engine->world, _engine->camera);

			_engine->camera->x = width();
			_engine->camera->y = height();
		} 

		updateGL();
  }

public slots: 
    virtual void setButtonMode(int mode) {
		_buttonMode = mode;
    }
  
protected:
    void initializeGL();
    void paintGL();
    void resizeGL( int w, int h );

	void mousePressEvent ( QMouseEvent *e);
	void mouseMoveEvent ( QMouseEvent *e );

private:
    brEngine *_engine;    
	int _buttonMode;
	QPoint lastPosition;
};

#endif // GLFRACTAL_H
#ifndef INSERT_POSITION_HH_
#define INSERT_POSITION_HH_ 1

#include <qwidget.h>

class DataStore;

class InsertPosition : public QWidget
{
  Q_OBJECT

public:
  InsertPosition(DataStore * datastore, QWidget * parent, const char * name);

  void paintEvent(QPaintEvent * e);

public slots:
  void setVisibleRange(int, double);
  void setScaffoldCoordinate(int);

private:
  DataStore * m_datastore;
  double m_scale;
  int m_start;
  int m_pos;

};

#endif

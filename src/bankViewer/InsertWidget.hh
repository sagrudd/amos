#ifndef INSERT_WIDGET_HH_
#define INSERT_WIDGET_HH_ 1

#include <qwidget.h>
#include <string>

#include "InsertField.hh"
#include "InsertCanvas.hh"
#include "InsertPosition.hh"
#include "DataStore.hh"

#include <qcanvas.h>
#include <qslider.h>

#include <qpixmap.h>
#include <qcanvas.h>
#include <qwidget.h>
#include <string>
#include "foundation_AMOS.hh"
#include "Insert.hh"
#include "DataStore.hh"



class InsertWidget : public QWidget
{
  Q_OBJECT

public:

  InsertWidget(DataStore * datastore,
               QWidget * parent = 0, const char * name = 0);
  ~InsertWidget();

public slots:
  void setTilingVisibleRange(int, int);
  void setZoom(int);
  void refreshCanvas();

signals:
  void setStatus(const QString & message);
  void setGindex(int gindex);


private:
  void flushInserts();
  void initializeVisibleRectangle();

  InsertField    * m_ifield;
  QCanvas        * m_icanvas;
  InsertPosition * m_iposition;

  QCanvasRectangle * m_tilingVisible;
  QSlider * m_zoom;

  DataStore * m_datastore;

  // from insert canvas
  void drawTile(AMOS::Tile_t * tile, QCanvas * p, int hoffset, int vpos, Insert::MateState state);
  AMOS::Distribution_t getLibrarySize(AMOS::ID_t readid);

  int m_seqheight;
  int m_hoffset;

  std::vector<AMOS::Tile_t> m_tiling;
  std::vector<Insert *> m_inserts;
};

#endif

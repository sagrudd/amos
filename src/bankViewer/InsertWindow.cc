#include "InsertWindow.hh"
#include "InsertWidget.hh"

#include <qstatusbar.h>
#include <qlabel.h>
#include <qslider.h>
#include <qpopupmenu.h>
#include <qtoolbutton.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <qaccel.h>
#include <qmenubar.h>
#include <qpixmap.h>
#include <qpainter.h>
#include <qlineedit.h>

#include "DataStore.hh"
#include "UIElements.hh"
#include "BufferedLineEdit.hh"


using namespace std;

InsertWindow::InsertWindow(DataStore * datastore,
                           QWidget * parent,
                           const char * name)
  : QMainWindow(parent, name),
    m_datastore(datastore)
{
  resize(800,600);
  setCaption("AI - Inserts");

  statusBar()->message("Ready", 2000);
  statusBar()->setSizeGripEnabled(false);

  // Toolbar
  QToolBar * options = new QToolBar(this, "options");
  options->setLabel("Options");

  new QLabel("Zoom", options, "zoomlbl");
  QSlider * zoom = new QSlider(1, 80, 4, 16, Qt::Horizontal, options, "zoom");

  new QLabel(" VZoom", options, "vzoomlbl");
  QSlider * vzoom = new QSlider(1, 40, 4, 16, Qt::Horizontal, options, "vzoom");

  new QLabel(" Happy Distance:", options, "happylbl");
  m_happypick = new QLineEdit(options, "happypick");
  m_happypick->setText(QString::number(Insert::MAXSTDEV));

  new QLabel(" IID:", options, "iidlbl");
  BufferedLineEdit * iidpick = new BufferedLineEdit(options, "iidpick");

  new QLabel(" EID:", options, "eidlbl");
  BufferedLineEdit * eidpick = new BufferedLineEdit(options, "eidpick");

  const char * states = Insert::allstates;
  unsigned int type = 0;

  // Libraries
  m_libmenu = new QPopupMenu(this);
  menuBar()->insertItem("&Libraries", m_libmenu);
  buildLibraryMenu();


  QPopupMenu * m_featmenu = new QPopupMenu(this);
  menuBar()->insertItem("&Features", m_featmenu);

  for (type = 0; type < strlen(UIElements::allFeatureTypes); type++)
  {
    char feat = UIElements::allFeatureTypes[type];
    QPixmap rect(10,10);

    QPainter p(&rect);
    p.fillRect(rect.rect(), UIElements::getFeatureColor((AMOS::FeatureType_t)feat));
    p.end();

    QString name = (QString)"[" + feat + "] ";
    name += UIElements::getFeatureStr((AMOS::FeatureType_t)feat);

    m_featmenu->insertItem(QIconSet(rect), name);
  }

  // Display Types
  m_typesmenu = new QPopupMenu(this);
  menuBar()->insertItem("&Mate Types", m_typesmenu);

  for (unsigned int i = 0; i < strlen(states); i++)
  {
    char state = states[i];
    QPixmap rect(10,10);

    QPainter p(&rect);
    p.fillRect(rect.rect(), UIElements::getInsertColor((Insert::MateState)state));
    p.end();

    QString name = (QString)"[" + state + "] ";
    name += Insert::getInsertTypeStr((Insert::MateState)state);

    m_types[state].first = m_typesmenu->insertItem(QIconSet(rect), name);
    m_types[state].second = true;
    m_typesmenu->setItemChecked(m_types[state].first, m_types[state].second);
  }

  // Options
  m_optionsmenu = new QPopupMenu(this);
  menuBar()->insertItem("&Options", m_optionsmenu);
  m_scaffid = m_optionsmenu->insertItem("&Scaffold Plot", this, SLOT(togglePaintScaffold()));
  m_optionsmenu->setItemChecked(m_scaffid, true);

  m_showscaffid = m_optionsmenu->insertItem("Show Scaffol&d", this, SLOT(toggleShowScaffold()));
  m_optionsmenu->setItemChecked(m_showscaffid, true);

  m_optionsmenu->insertSeparator();

  m_coverageid = m_optionsmenu->insertItem("Coverage Plo&t", this, SLOT(toggleCoveragePlot()));
  m_optionsmenu->setItemChecked(m_coverageid, true);

  m_ceid = m_optionsmenu->insertItem("C&E Statisitic", this, SLOT(toggleCEStatistic()));
  m_optionsmenu->setItemChecked(m_ceid, false);

  m_featid = m_optionsmenu->insertItem("Show &Features", this, SLOT(toggleFeatures()));
  m_optionsmenu->setItemChecked(m_featid, true);

  m_optionsmenu->insertSeparator();

  m_connectmatesid = m_optionsmenu->insertItem("&Connect Mates", this, SLOT(toggleConnectMates()));
  m_optionsmenu->setItemChecked(m_connectmatesid, true);

  m_partitiontypesid = m_optionsmenu->insertItem("&Partition Types", this, SLOT(togglePartitionTypes()));
  m_optionsmenu->setItemChecked(m_partitiontypesid, true);

  m_optionsmenu->insertSeparator();

  m_libcolorid = m_optionsmenu->insertItem("Color By &Library", this, SLOT(toggleColorByLibrary()));
  m_optionsmenu->setItemChecked(m_libcolorid, false);

  m_matecolorid = m_optionsmenu->insertItem("Color By Lin&ked Contig", this, SLOT(toggleColorByMate()));
  m_optionsmenu->setItemChecked(m_matecolorid, false);

  // Main Widget
  InsertWidget * iw = new InsertWidget(datastore, m_types, this, "iw");
  setCentralWidget(iw);

  connect(m_typesmenu, SIGNAL(activated(int)),
          this,        SLOT(toggleItem(int)));

  connect(iw,          SIGNAL(setStatus(const QString &)),
          statusBar(), SLOT(message(const QString &)));

  connect(iw,   SIGNAL(setGindex(int)),
          this, SIGNAL(setGindex(int)));

  connect(iw,   SIGNAL(setContigId(int)),
          this, SIGNAL(setContigId(int)));

  connect(this, SIGNAL(setTilingVisibleRange(int, int, int)),
          iw,   SLOT(setTilingVisibleRange(int, int, int)));

  connect(zoom, SIGNAL(valueChanged(int)),
          iw,   SLOT(setZoom(int)));

  connect(vzoom, SIGNAL(valueChanged(int)),
          iw,   SLOT(setVZoom(int)));

  connect(this, SIGNAL(paintCanvas()),
          iw,   SLOT(paintCanvas()));

  connect(iidpick, SIGNAL(newValue(const QString &)),
          iw,      SIGNAL(highlightIID(const QString &)));

  connect(eidpick, SIGNAL(newValue(const QString &)),
          iw,      SIGNAL(highlightEID(const QString &)));

  connect(m_happypick, SIGNAL(returnPressed()),
          this,        SLOT(loadHappyDistance()));

  connect(this,        SIGNAL(setShowScaffold(bool)),
          iw,          SLOT(setShowScaffold(bool)));

  connect(this,        SIGNAL(setHappyDistance(float)),
          iw,          SLOT(setHappyDistance(float)));

  connect(this, SIGNAL(setConnectMates(bool)),
          iw,   SLOT(setConnectMates(bool)));

  connect(this, SIGNAL(setPartitionTypes(bool)),
          iw,   SLOT(setPartitionTypes(bool)));

  connect(this, SIGNAL(setCoveragePlot(bool)),
          iw,   SLOT(setCoveragePlot(bool)));

  connect(this, SIGNAL(setTintHappiness(bool)),
          iw,   SLOT(setTintHappiness(bool)));

  connect(this, SIGNAL(setTintFeatures(bool)),
          iw,   SLOT(setTintFeatures(bool)));

  connect(this, SIGNAL(setCEStatistic(bool)),
          iw,   SLOT(setCEStatistic(bool)));

  connect(this, SIGNAL(setPaintScaffold(bool)),
          iw,   SLOT(setPaintScaffold(bool)));

  connect(this, SIGNAL(setFeatures(bool)),
          iw,   SLOT(setFeatures(bool)));

  connect(this, SIGNAL(setColorByLibrary(bool)),
          iw,   SLOT(setColorByLibrary(bool)));

  connect(this, SIGNAL(setColorByMate(bool)),
          iw,   SLOT(setColorByMate(bool)));

  connect(this, SIGNAL(newContig()),
          iw,   SLOT(contigChanged()));

  connect(this, SIGNAL(highlightRead(int)),
          iw,   SIGNAL(highlightRead(int)));

  connect(parent, SIGNAL(bankSelected()),
          iw,   SLOT(refreshWidget()));

  connect(iw, SIGNAL(readIIDHighlighted(const QString &)),
          iidpick, SLOT(setText(const QString &)));

  connect(iw, SIGNAL(readEIDHighlighted(const QString &)),
          eidpick, SLOT(setText(const QString &)));

  connect(iw, SIGNAL(jumpToRead(int)),
          parent, SLOT(jumpToRead(int)));


  zoom->setValue(32);

  if (s_persistant)
  {
    // Persistant Toolbar
    QToolBar * persistant = new QToolBar(this, "persistant");
    persistant->setLabel("Persistant");

    new QLabel("Persistant", persistant, "persistlbl");
    QCheckBox * persist = new QCheckBox(persistant, "persistantcheck");

    new QLabel("Error rate", persistant, "eratelbl");
    QSpinBox * spin = new QSpinBox(0,19,1,persistant, "eratespin");

    connect(persist, SIGNAL(toggled(bool)),
            iw,      SLOT(setPersistant(bool)));
    connect(spin,    SIGNAL(valueChanged(int)),
            iw,      SLOT(setErrorRate(int)));
  }
}

int InsertWindow::s_persistant(0);

void InsertWindow::buildLibraryMenu()
{
  m_libmenu->clear();

  unsigned int type = 0;
  const char * states = Insert::allstates;

  DataStore::LibLookup_t::iterator li;
  for (li =  m_datastore->m_libdistributionlookup.begin();
       li != m_datastore->m_libdistributionlookup.end();
       li++)
  {
    char state = states[type];
    QPixmap rect(10,10);

    QPainter p(&rect);
    p.fillRect(rect.rect(), UIElements::getInsertColor((Insert::MateState)state));
    p.end();

    QString name = QString::number(li->first); 
    name += " [" + QString::number(li->second.mean); 
    name += " +/- " + QString::number(li->second.sd);
    name += "]";

    m_libmenu->insertItem(QIconSet(rect), name);

    type++;
    if (type >= strlen(Insert::allstates)) { type = 0; }
  }
}


void InsertWindow::loadHappyDistance()
{
  emit setHappyDistance(atof(m_happypick->text().ascii()));
}

void InsertWindow::contigChanged()
{
  emit newContig();
}

void InsertWindow::toggleItem(int id)
{
  typemap::iterator mi;

  for (mi = m_types.begin(); mi != m_types.end(); mi++)
  {
    if (mi->second.first == id)
    {
      mi->second.second = !m_typesmenu->isItemChecked(mi->second.first);
      m_typesmenu->setItemChecked(mi->second.first, mi->second.second);

      emit paintCanvas();
      break;
    }
  }
}

void InsertWindow::toggleConnectMates()
{
  bool b = !m_optionsmenu->isItemChecked(m_connectmatesid);
  m_optionsmenu->setItemChecked(m_connectmatesid, b);

  emit setConnectMates(b);
}

void InsertWindow::togglePartitionTypes()
{
  bool b = !m_optionsmenu->isItemChecked(m_partitiontypesid);
  m_optionsmenu->setItemChecked(m_partitiontypesid, b);

  emit setPartitionTypes(b);
}

void InsertWindow::toggleCoveragePlot()
{
  bool b = !m_optionsmenu->isItemChecked(m_coverageid);
  m_optionsmenu->setItemChecked(m_coverageid, b);

  emit setCoveragePlot(b);
}

void InsertWindow::toggleCEStatistic()
{
  bool b = !m_optionsmenu->isItemChecked(m_ceid);
  m_optionsmenu->setItemChecked(m_ceid, b);

  emit setCEStatistic(b);
}

void InsertWindow::togglePaintScaffold()
{
  bool b = !m_optionsmenu->isItemChecked(m_scaffid);
  m_optionsmenu->setItemChecked(m_scaffid, b);

  emit setPaintScaffold(b);
}

void InsertWindow::toggleFeatures()
{
  bool b = !m_optionsmenu->isItemChecked(m_featid);
  m_optionsmenu->setItemChecked(m_featid, b);

  emit setFeatures(b);
}

void InsertWindow::toggleTintHappiness()
{
  bool b = !m_optionsmenu->isItemChecked(m_tintid);
  m_optionsmenu->setItemChecked(m_tintid, b);

  emit setTintHappiness(b);
}

void InsertWindow::toggleTintFeatures()
{
  bool b = !m_optionsmenu->isItemChecked(m_tintfeatid);
  m_optionsmenu->setItemChecked(m_tintfeatid, b);

  emit setTintFeatures(b);
}

void InsertWindow::toggleColorByLibrary()
{
  bool b = !m_optionsmenu->isItemChecked(m_libcolorid);
  m_optionsmenu->setItemChecked(m_libcolorid, b);

  emit setColorByLibrary(b);
}

void InsertWindow::toggleColorByMate()
{
  bool b = !m_optionsmenu->isItemChecked(m_matecolorid);
  m_optionsmenu->setItemChecked(m_matecolorid, b);

  emit setColorByMate(b);
}

void InsertWindow::toggleShowScaffold()
{
  bool b = !m_optionsmenu->isItemChecked(m_showscaffid);
  m_optionsmenu->setItemChecked(m_showscaffid, b);

  emit setShowScaffold(b);
}

void InsertWindow::bankChanged()
{
  buildLibraryMenu();
}

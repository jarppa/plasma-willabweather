/*
 *   Author: Jari Tervonen <jjtervonen@gmail.com>                               
 *   
 *   Adapted from Plasma-simpleweatherforecast by Karthik Paithankar.
 * 
 *   This program is free software; you can redistribute it and/or modify  
 *   it under the terms of the GNU General Public License as published by  
 *   the Free Software Foundation; either version 3 of the License, or     
 *   (at your option) any later version.                                   
 *                                                                         
 *   This program is distributed in the hope that it will be useful,       
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         
 *   GNU General Public License for more details.                          
 *                                                                         
 *   You should have received a copy of the GNU General Public License     
 *   along with this program; if not, write to the                         
 *   Free Software Foundation, Inc.,                                       
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        
 */

#ifndef Plasma_Weather_HEADER
#define Plasma_Weather_HEADER

#include <Plasma/Applet>
#include <Plasma/Svg>
#include <QTimer>
#include <QPair>
#include <QList>
#include <KConfigDialog>

#include "ui_config.h"
#include "config.h"

class QSizeF;
class QTimer;

class Plasma_Weather : public Plasma::Applet
{
    Q_OBJECT
    public:
        Plasma_Weather(QObject *parent, const QVariantList &args);
        ~Plasma_Weather();

        void paintInterface(QPainter *painter,
                const QStyleOptionGraphicsItem *option,
                const QRect& contentsRect);
        void init();
        QSizeF contentSizeHint() const;
        Qt::Orientations expandingDirections() const;

    public slots:
        void refresh();
        void animate();
        void configAccepted();
        void configRejected();

    protected:
        void createConfigurationInterface(KConfigDialog *parent);

    private:
        void collectData();
        void readConfigData();
        void parseData();
        void parseWillab();
        void collectAnimationFrames();
        
        QString attribution_text;
        QString updateFrequency;
        QString animFrequency;
        QString source;
        QString image_source;
        QString forecast_source;
        QString animation_source_s;
        QString animation_source_e;
        QList< QPair<QString *,QImage *> *> animation_frames;
        
        QString meastime;
        QPair<QString,QString> windspeed;
        QPair<QString,QString> windchill;
        QPair<QString,QString> dewpoint;
        QString geoloc;
        QPair<QString,QString> tempnow;
        QPair<QString,QString> temphi;
        QPair<QString,QString> templo;
        QPair<QString,QString> humidity;
        QPair<QString,QString> airpressure;
        QPair<QString,QString> windspeedmax;
        QPair<QString,QString> precipitation_1d;
        QPair<QString,QString> precipitation_1h;
        QPair<QString,QString> winddir;
        QString solarrad;
        QString attribution;
        
        QTimer* timer;
        QTimer* animation_timer;
        QFont small_font;
        QFont big_font;
        ConfigDialog* conf;
        bool animating;
        bool updating;
        int current_anim_frame;
};
 
K_EXPORT_PLASMA_APPLET(simpleweatherforecast, Plasma_Weather)
#endif 

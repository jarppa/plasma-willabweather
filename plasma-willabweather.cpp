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

#include "plasma-willabweather.h"
#include "config.h"

#include <QPainter>
#include <QFontMetrics>
#include <QStyleOptionGraphicsItem>
#include <QSizeF>
#include <QRectF>
#include <QFile>
#include <KGlobalSettings>
#include <QProcess>
#include <QDomDocument>
#include <QTimer>
#include <QDir>
#include <QLineF>
#include <QList>
#include <QPair>
#include <QDateTime>
#include <KConfigGroup>

#include <plasma/svg.h>
#include <plasma/theme.h>
#include <kio/job.h>


Plasma_Weather::Plasma_Weather(
    QObject *parent, const QVariantList &args  )
    : Plasma::Applet(parent, args)
{
   timer = new QTimer(this);
   animation_timer = new QTimer(this);
   
   resize(370, 700);

   setHasConfigurationInterface(true);

   connect(timer, SIGNAL(timeout()), this, SLOT(refresh()));
   connect(animation_timer, SIGNAL(timeout()), this, SLOT(animate()));
}
 
Plasma_Weather::~Plasma_Weather()
{
    timer->stop();
    animation_timer->stop();
    delete timer;
    delete animation_timer;
    while (!animation_frames.isEmpty()) {
        QPair<QString *, QImage *> *temp = animation_frames.takeFirst();
        delete temp->first;
        delete temp->second;
    }
}

void Plasma_Weather::init()
{
    //qDebug() << "init";
    attribution = "VTT (http://www.vtt.fi), Foreca (http://www.foreca.com)";
    current_anim_frame = 0;
    animating = TRUE;
    updating = FALSE;
    readConfigData();
    refresh();
}

void Plasma_Weather::animate()
{
    //qDebug() << "animate";
    current_anim_frame = current_anim_frame < 23 ? current_anim_frame+1 : 0;
    update();
}

void Plasma_Weather::refresh()
{
    //qDebug() << "refresh";
    bool error = false;
    
    animation_timer->stop();
    animation_timer->setInterval(animFrequency.toInt(&error) * 1000);
    
    timer->stop();
    timer->setInterval(updateFrequency.toInt(&error) * 60 * 1000);
    
    updating = true;
    update();
    
    collectData();
    parseData();
    
    collectAnimationFrames();
    
    updating = false;
    update();
    
    if (animating)
        animation_timer->start();
   
    timer->start();
}

void Plasma_Weather::readConfigData()
{
   KConfigGroup cg = config();

   source  = cg.readEntry("source", "http://weather.willab.fi/weather.xml");
   image_source = cg.readEntry("image_source", "http://www.ipv6.willab.fi/weather/temperature_small.png");
   forecast_source = cg.readEntry("forecast_source", "http://foreca.fi/meteogram.php?loc_id=100643492&lang=fi");
   animation_source_s = cg.readEntry("animation_source_s", "http://cache-");
   animation_source_e = cg.readEntry("animation_source_e", ".foreca.com/i/radext2/mfi2-radext2-");
   //qDebug() << animation_source_s + animation_source_e;
   updateFrequency = cg.readEntry("updateFrequency", "10");
   animFrequency = cg.readEntry("animFrequency", "1");
   small_font.fromString( cg.readEntry("small_font", QFont(KGlobalSettings::smallestReadableFont()).toString()) );
   big_font.fromString( cg.readEntry("big_font", QFont(KGlobalSettings::largeFont()).toString()) );
} 
 
void Plasma_Weather::collectData()
{
    KIO::file_delete(QDir::tempPath()+"/.willabweather.xml", KIO::HideProgressInfo );
    //qDebug() << "Collect";
    
    QProcess* process = new QProcess(this);
    
    QString wget_xml_command = "wget -q -t 4 --random-wait -O "+
                    QDir::tempPath()+"/.willabweather.xml "+source;
                    
    QString wget_image_command = "wget -q -t 4 -O "+QDir::tempPath()+"/weatherimage "+image_source;
    
    QString wget_forecastimage_command = "wget -q -t 4 -O "+QDir::tempPath()+"/forecastimage "+forecast_source;
    
    //qDebug() << wget_xml_command;
    //qDebug() << wget_image_command;
    //qDebug() << wget_forecastimage_command;
    
    process->start(wget_xml_command);
    process->waitForFinished();
    
    process->start(wget_image_command);
    process->waitForFinished();
    
    process->start(wget_forecastimage_command);
    process->waitForFinished();
    
    delete process;
}

void Plasma_Weather::collectAnimationFrames()
{
    //qDebug() << "Collect Animation Frames";
    QProcess* process = new QProcess(this);
    
    QDateTime dt = QDateTime::currentDateTimeUtc();
    QString url_ext = dt.toString("yyyyMMdd");
    QString wget_anim_command;
    QString frameid;
    QString frame_path;
    QString frame_url;
    
    while (!animation_frames.isEmpty()) {
        QPair<QString *, QImage *> *temp = animation_frames.takeFirst();
        delete temp->first;
        delete temp->second;
    }
    
    char sources [] = {'a','b','c'}; //caches
    
    for (int i=0; i<24;i++) {
        
        for (int s=0; s<3;s++) {
            if (i < 10) {
                frameid = url_ext;
                frameid += "0";
                frameid += QString::number(i);
                frameid += "0000";
            }
            else {
                frameid = url_ext; //yyyyMMdd
                frameid += QString::number(i); //hh
                frameid += "0000"; //mmss
            }
        
            frame_path = QDir::tempPath()+"/.willabweather_animation"+QString::number(i)+".png";
            //qDebug() << frame_path;
            frame_url = animation_source_s+sources[s]+animation_source_e+frameid+".png";
            //qDebug() << frame_url;
      
            wget_anim_command = "wget -q -t 4 --random-wait -O "+frame_path+" "+frame_url;
            //qDebug() << wget_anim_command;
        
            process->start(wget_anim_command);
            process->waitForFinished();
            
            QFile *f = new QFile(frame_path);
            
            if (f->size() != 0) {
                QPair<QString *,QImage *> *pari = new QPair<QString *,QImage *>(new QString(frameid),new QImage(frame_path));
                animation_frames << pari;
                delete f;
                break;
            }
            else {
                delete f;
                continue;
            }
        }
    }
    delete process;
}

void Plasma_Weather::parseData()
{
    //qDebug() << "parse";
    parseWillab();
}

void Plasma_Weather::parseWillab()
{
    //qDebug() << "parsewillab";
    QDomDocument doc("weather");
    QFile file(QDir::tempPath() + "/.willabweather.xml");
    doc.setContent(&file);
    file.close();
    
    QDomElement docElem = doc.documentElement();
    
    QDomNode n = docElem.firstChild();
    QString tname;
    
    //TODO: can have multiple weathers in one weatherpage?
    while(!n.isNull())
    {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        if(!e.isNull() && e.tagName() == "weather")
        {
            //qDebug() << e.tagName();
            n = n.firstChild();
            
            while(!n.isNull())
            {
                QDomElement e = n.toElement();
                tname = e.tagName();
                //qDebug() << tname;
                if(tname == "location")
                    geoloc = e.attribute("value", "");
                else if(tname == "time")
                    meastime = e.text();
                else if(tname == "solarrad")
                    solarrad = e.text();
                else if(tname == "tempnow")
                {
                    tempnow.first = e.text();
                    tempnow.second = e.attribute("unit");
                }
                else if(tname == "temphi")
                {
                    temphi.first = e.text();
                    temphi.second = e.attribute("unit");
                }
                else if(tname == "templo")
                {
                    templo.first = e.text();
                    templo.second = e.attribute("unit");
                }
                else if(tname == "dewpoint")
                {
                    dewpoint.first = e.text();
                    dewpoint.second = e.attribute("unit");
                }
                else if(tname == "humidity")
                {
                    humidity.first = e.text();
                    humidity.second = e.attribute("unit");
                }
                else if(tname == "airpressure")
                {
                    airpressure.first = e.text();
                    airpressure.second = e.attribute("unit");
                }
                else if(tname == "windspeed")
                {
                    windspeed.first = e.text();
                    windspeed.second = e.attribute("unit");
                }
                else if(tname == "windspeedmax")
                {
                    windspeedmax.first = e.text();
                    windspeedmax.second = e.attribute("unit");
                }
                else if(tname == "winddir")
                {
                    winddir.first = e.text();
                    winddir.second = e.attribute("unit");
                }
                else if(tname == "precipitation")
                {
                    QString t = e.attribute("time");
                    if (!t.compare("1d"))
                    {
                        precipitation_1d.first = e.text();
                        precipitation_1d.second = e.attribute("unit");
                    }
                    else if (!t.compare("1h"))
                    {
                        precipitation_1h.first = e.text();
                        precipitation_1h.second = e.attribute("unit");
                    }
                }
                else if(tname == "windchill")
                {
                    windchill.first = e.text();
                    windchill.second = e.attribute("unit");
                }
                
                n = n.nextSibling();
            }
            return;
            n = n.parentNode();
            n = n.nextSibling();
        }
        n = n.nextSibling();
    }
}

void Plasma_Weather::paintInterface(QPainter *p,
        const QStyleOptionGraphicsItem *option, const QRect &contentsRect)
{
    Q_UNUSED( option );

    int DIVIDE_NUM = 16;
    
    QRect r_data(contentsRect.left(),
                    contentsRect.top(),
                    contentsRect.width(),
                    (int)((contentsRect.height()-60)/3));
    
    QRect r_geoloc((int)r_data.width()/2, 
                    r_data.top(), 
                    (int)(r_data.width()/2), 
                    (int)(r_data.height()/DIVIDE_NUM));
    
    QRect r_image(r_data.left(), 
                    r_data.top()+10,
                    (int)(r_data.width()/2),
                    (int)r_data.height()/2);
    
    QRect r_forecast(r_data.left(),
                        r_data.height()+20,
                        contentsRect.width(),
                        (int)((contentsRect.height()-60)/3));
    
    QRect r_rain(r_data.left(),
                        r_forecast.bottom()+20,
                        contentsRect.width(),
                        (int)((contentsRect.height()-60)/3));
    
    QRect r_anim_time(r_rain.left(),
                        r_rain.bottom(),
                        r_rain.width(), 20);
    
    QRect r_attribution(r_rain.left(),
                        r_anim_time.bottom(),
                        r_rain.width(), 20);
                        
    QRect r_tempnow(r_data.left(), 
                    (int)(r_image.bottom()),
                    (int)(r_data.width()/2),
                    (int)(r_data.height()-r_image.bottom()));
                    
    QRect r_temphi(r_data.width()/2, 
                    (int)(r_data.top()+(r_data.height()/DIVIDE_NUM)),
                    (int)r_data.width()/2,
                    (int)r_data.height()/DIVIDE_NUM);
                    
    QRect r_templo((int)r_data.width()/2, 
                    r_data.top()+(int)(2*r_data.height()/DIVIDE_NUM),
                    (int)(r_data.width()/2),
                    (int)r_data.height()/DIVIDE_NUM);

    QRect r_windspeed((int)r_data.width()/2, 
                    r_data.top()+(int)(3*r_data.height()/DIVIDE_NUM),
                    (int)(r_data.width()/2), 
                    (int)r_data.height()/DIVIDE_NUM);
                    
    QRect r_winddir((int)r_data.width()/2, 
                    r_data.top()+(int)(4*r_data.height()/DIVIDE_NUM),
                    (int)(r_data.width()/2), 
                    (int)r_data.height()/DIVIDE_NUM);
                    
    QRect r_windchill((int)r_data.width()/2, 
                    r_data.top()+(int)(5*r_data.height()/DIVIDE_NUM),
                    (int)(r_data.width()/2), 
                    (int)r_data.height()/DIVIDE_NUM);
    
    QRect r_windspeedmax((int)r_data.width()/2, 
                    r_data.top()+(int)(6*r_data.height()/DIVIDE_NUM),
                    (int)(r_data.width()/2), 
                    (int)r_data.height()/DIVIDE_NUM);
                    
    QRect r_dewpoint((int)r_data.width()/2, 
                    r_data.top()+(int)(7*r_data.height()/DIVIDE_NUM),
                    (int)(r_data.width()/2), 
                    (int)r_data.height()/DIVIDE_NUM);
                    
    QRect r_humidity((int)r_data.width()/2, 
                    r_data.top()+(int)(8*r_data.height()/DIVIDE_NUM),
                    (int)(r_data.width()/2), 
                    (int)r_data.height()/DIVIDE_NUM);
                    
    QRect r_airpressure((int)r_data.width()/2, 
                    r_data.top()+(int)(9*r_data.height()/DIVIDE_NUM),
                    (int)(r_data.width()/2), 
                    (int)r_data.height()/DIVIDE_NUM);
                    
    QRect r_preci1d((int)r_data.width()/2, 
                    r_data.top()+(int)(10*r_data.height()/DIVIDE_NUM),
                    (int)(r_data.width()/2), 
                    (int)r_data.height()/DIVIDE_NUM);
    
    QRect r_preci1h((int)r_data.width()/2, 
                    r_data.top()+(int)(11*r_data.height()/DIVIDE_NUM),
                    (int)r_data.width()/2,
                    (int)r_data.height()/DIVIDE_NUM);
    
    QRect r_solarrad((int)r_data.width()/2, 
                    r_data.top()+(int)(12*r_data.height()/DIVIDE_NUM),
                    (int)r_data.width()/2,
                    (int)r_data.height()/DIVIDE_NUM);
                    
    QRect r_meastime((int)r_data.width()/2, 
                    r_data.top()+(int)(14*r_data.height()/DIVIDE_NUM),
                    (int)r_data.width()/2,
                    (int)r_data.height()/DIVIDE_NUM);
    
    /*QRect r_attribution((int)r_data.width()/2, 
                    r_data.top()+(int)(15*r_data.height()/DIVIDE_NUM),
                    (int)r_data.width()/2,
                    (int)r_data.height()/DIVIDE_NUM);*/
                    
    p->setRenderHint(QPainter::SmoothPixmapTransform);
    p->setRenderHint(QPainter::Antialiasing);
 
    p->save();
    
    QImage image(QDir::tempPath()+"/weatherimage",0);
    if (!image.isNull())
    {
        p->drawImage(r_image,image,image.rect(),Qt::AutoColor);
    }
    
    if (animating && !updating)
    {
        //qDebug() << "animate";
        //qDebug() << current_anim_frame;
        //qDebug() << animation_frames.size();
        if (animation_frames.size()) {
            QImage *rimage = animation_frames[current_anim_frame]->second;
            if (!rimage->isNull()) {
                //qDebug() << "Not NULL";
                p->drawImage(r_rain,*rimage,rimage->rect(),Qt::AutoColor);
            }
            //else
                //qDebug() << "Is NULL";
        }
    }
    
    QImage fimage(QDir::tempPath()+"/forecastimage",0);
        if (!fimage.isNull())
            p->drawImage(r_forecast,fimage,fimage.rect(),Qt::AutoColor);
    
    p->setPen(Qt::NoPen);
    p->setPen(Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor));
    
    p->setFont(small_font);
    
    p->drawText(r_geoloc,
                Qt::AlignVCenter| Qt::AlignHCenter,
                "Location: "+geoloc); 
                
    p->setFont(big_font);
    
    p->drawText(r_tempnow,
                Qt::AlignVCenter| Qt::AlignHCenter,
                tempnow.first+" "+tempnow.second);
    if (updating)
        p->drawText(r_rain,
                Qt::AlignVCenter| Qt::AlignHCenter,
                "UPDATING");
    
    p->setFont(small_font);
    
    p->drawText(r_temphi,
                Qt::AlignVCenter| Qt::AlignHCenter,
                "High: "+temphi.first+" "+temphi.second);
                
    p->drawText(r_templo,
                Qt::AlignVCenter| Qt::AlignHCenter,
                "Low: "+templo.first+" "+templo.second);
                 
    p->drawText(r_windspeed,
                Qt::AlignVCenter| Qt::AlignHCenter,
                "Wind speed: "+windspeed.first+" "+windspeed.second);
                
    p->drawText(r_winddir,
                Qt::AlignVCenter| Qt::AlignHCenter,
                "Wind direction:"+winddir.first+" "+winddir.second);
               
    if (windchill.first == "") windchill.first = "n/a";
     
    p->drawText(r_windchill,
                Qt::AlignVCenter| Qt::AlignHCenter,
                "Wind chill: "+windchill.first+" "+windchill.second);
                
    p->drawText(r_windspeedmax,
                Qt::AlignVCenter| Qt::AlignHCenter,
                "Wind speed (max): "+windspeedmax.first+" "+windspeedmax.second);
                
    p->drawText(r_dewpoint,
                Qt::AlignVCenter| Qt::AlignHCenter,
                "Dewpoint: "+dewpoint.first+" "+dewpoint.second);
                
    p->drawText(r_humidity,
                Qt::AlignVCenter| Qt::AlignHCenter,
                "Humidity: "+humidity.first+" "+humidity.second);
                
    p->drawText(r_airpressure,
                Qt::AlignVCenter| Qt::AlignHCenter,
                "Air pressure: "+airpressure.first+" "+airpressure.second);
    
    p->drawText(r_preci1d,
                Qt::AlignVCenter| Qt::AlignHCenter,
                "Precipitation (1d): "+precipitation_1d.first+" "+precipitation_1d.second);
                
    p->drawText(r_preci1h,
                Qt::AlignVCenter| Qt::AlignHCenter,
                "Precipitation (1h): "+precipitation_1h.first+" "+precipitation_1h.second);
    
    if (solarrad == "") solarrad = "n/a";
    p->drawText(r_solarrad,
                Qt::AlignVCenter| Qt::AlignHCenter,
                "Solar rad: "+solarrad);
                
    p->drawText(r_meastime,
                Qt::AlignVCenter| Qt::AlignHCenter,
                "Measured: "+meastime);
    
    QString timeline;
    if (current_anim_frame < 10)
        timeline = "0"+QString::number(current_anim_frame)+":00";
    else
        timeline = QString::number(current_anim_frame)+":00";
    
    p->drawText(r_anim_time,
                Qt::AlignVCenter| Qt::AlignHCenter,
                timeline);
    
    p->drawText(r_attribution,
                Qt::AlignVCenter| Qt::AlignHCenter,
                "Data from: "+attribution);
                
        /*if(iconTheme == "")
        {
            svg_file = "widgets/plasma-simpleweatherforecast";
        }
        else
        {
            svg_file = iconTheme;
        }*/

        //Plasma::Svg* svg;
        //svg = new Plasma::Svg(this);
        //svg->setImagePath( svg_file );
        //svg->setContainsMultipleImages(true);
        //svg->resize(imageRect.width(), imageRect.height());
        //svg->resize(imageRect_tomorrow.width(), imageRect_tomorrow.height());
        //svg->resize(imageRect_d_a_t.width(), imageRect_d_a_t.height());
        //svg->paint(p,imageRect, today_code);
        //svg->paint(p,imageRect_tomorrow, tomorrow_code);
        //svg->paint(p,imageRect_d_a_t, d_a_t_code);
        
    p->restore();
}

void Plasma_Weather::createConfigurationInterface(KConfigDialog* parent)
{
   bool error = false;
   conf = new ConfigDialog();
   parent->setButtons(KDialog::Ok | KDialog::Cancel | KDialog::Apply);
   parent->addPage(conf, parent->windowTitle(), icon());

   conf->freqChooser->setValue(updateFrequency.toInt(&error));
   conf->animFreqChooser->setValue(animFrequency.toInt(&error));
   conf->big_font = big_font;
   conf->small_font = small_font;
    
   connect(parent, SIGNAL(applyClicked()), this, SLOT(configAccepted()));
   connect(parent, SIGNAL(okClicked()), this, SLOT(configAccepted()));
   connect(parent, SIGNAL(cancelClicked()), this, SLOT(configRejected()));
}

void Plasma_Weather::configAccepted()
{
    KConfigGroup cg = config();

    cg.writeEntry("updateFrequency", updateFrequency.setNum(conf->freqChooser->value()));
    cg.writeEntry("animFrequency", animFrequency.setNum(conf->animFreqChooser->value()));
    cg.writeEntry("small_font", conf->small_font.toString());
    cg.writeEntry("big_font", conf->big_font.toString());
    emit configNeedsSaving();

    refresh();
   
    delete conf;
}

void Plasma_Weather::configRejected()
{
   delete conf;
}

Qt::Orientations Plasma_Weather::expandingDirections() const
{
    // no use of additional space in any direction
    return 0;
}

#include "plasma-willabweather.moc" 

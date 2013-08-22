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
 
#include "config.h"

#include "QFileDialog"
#include <QFontDialog>

ConfigDialog::ConfigDialog() : QWidget()
{
   setupUi(this);
   connect(bigFontButton, SIGNAL(clicked()), this, SLOT(getBigFont()));
   connect(smallFontButton, SIGNAL(clicked()), this, SLOT(getSmallFont()));
}

void ConfigDialog::getSmallFont()
{
    //  If user "cancels" dialog, do not change font
    bool ok = false;
    QFont newFont = QFontDialog::getFont(&ok, this);
    if(ok) small_font = newFont;
}

void ConfigDialog::getBigFont()
{
    //  If user "cancels" dialog, do not change font
    bool ok = false;
    QFont newFont = QFontDialog::getFont(&ok, this);
    if(ok) big_font = newFont;
}

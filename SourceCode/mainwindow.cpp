//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.

//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

//    Written by Julien ROBIN (julien.robin@pixconfig.fr) - 2017-12-16


#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //Just a little thing :
    QDate today = QDate::currentDate();
    ui->dateTimeEdit_from->setDate(today);
    ui->dateTimeEdit_to->setDate(today);

    QTime midnight(0, 0, 0);
    QTime lastSecond(23, 59, 59);
    ui->dateTimeEdit_from->setTime(midnight);
    ui->dateTimeEdit_to->setTime(lastSecond);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_Browse_clicked()
{
    QString currentText = ui->lineEdit_stversions->text();
    QString folder = QFileDialog::getExistingDirectory (this, ".stversions folder address", currentText, QFileDialog::ShowDirsOnly);

    //if used didn't canceled :
    if (folder.isEmpty() == false) ui->lineEdit_stversions->setText(folder);
}

//The main job is done here :
void MainWindow::on_pushButton_Start_clicked()
{
    QString stversionsFolderAddr = ui->lineEdit_stversions->text();

    if (stversionsFolderAddr.isEmpty() == true)
    {
        QMessageBox::information(this, NULL, "Please fill the .stversions folder's address");
        return;
    }

    QDir handyDir(stversionsFolderAddr);
    bool isFolderExisting = handyDir.exists();

    if (isFolderExisting == false)
    {
        QMessageBox::warning(this, NULL, "Check .stversions folder's address, the given folder does not seem to exist.");
        return;
    }

    //1 - Browse all files
    QDirIterator iterator(stversionsFolderAddr, QDir::NoDotAndDotDot|QDir::Files, QDirIterator::Subdirectories);
    QStringList allFilesList;
    while (iterator.hasNext() == true) allFilesList << iterator.next();

    //2 - Strip timestamp and .stversions on any matching file
    QList<QStringList> sourcesAndDestinations;
    for (auto currentFile : allFilesList)
    {
        //2.1 - check if the file should be included to the restore process
        bool matching = false; //Can become true after some tests

        if (ui->checkBox_restoreAll->isChecked() == true) matching = true;
        else
        {
            QDateTime minimumTime = ui->dateTimeEdit_from->dateTime();
            QDateTime maximumTime = ui->dateTimeEdit_to->dateTime();

            QDateTime currentFile_DateTime;
            bool bRet = getTimestamp(currentFile_DateTime, currentFile);

            if (bRet == false)
                QMessageBox::warning(this, NULL, "Unable to read the timestamp of " + currentFile);

            else if ((currentFile_DateTime >= minimumTime) && (currentFile_DateTime <= maximumTime))
            {
                //This file should be taken
                matching = true;
            }
        }

        //2.2 - find back and prepare the destination
        if (matching == true)
        {
            QString restoredFilePath = currentFile;
            bool bRet1 = stripTimestamp(restoredFilePath);
            bool bRet2 = stripStversions(restoredFilePath);

            QString destinationDir = restoredFilePath;
            bool bRet3 = stripFilename(destinationDir);
            bool bRet4 = handyDir.mkpath(destinationDir);

            if (bRet1 && bRet2 && bRet3 && bRet4)
            {
                QStringList sourceAndDestination;
                sourceAndDestination << currentFile;
                sourceAndDestination << restoredFilePath;

                sourcesAndDestinations.append(sourceAndDestination);
            }
            else
            {
                QMessageBox::warning(this, NULL, "Unable to prepare destination for " + currentFile);
            }
        }
    }

    //3 - This case should be taken into account before to continue
    if (sourcesAndDestinations.size() == 0)
    {
        QMessageBox::information(this, NULL, "Scan terminated, no file is matching your restore request. Nothing modified");
        return;
    }


    //4 - Check if there is several source for any destination file
    for (int i=0; i<sourcesAndDestinations.size(); i++)
    {
        bool conflict = false; //Can become true after some tests.

        for (int j=0; j<sourcesAndDestinations.size(); j++)
        {
            QString iDestination = sourcesAndDestinations[i][1];
            QString jDestination = sourcesAndDestinations[j][1];

            if ((i != j) && (iDestination == jDestination))
            {
                //if it's the first "j" match for this "i" :
                if (conflict == false)
                {
                    QString messageToDisplay = "Several files are matching to be restored at " + iDestination + "\n\n";
                    messageToDisplay += "Those files won't be processed, you will have to retry later with different settings (or manually)";
                    QMessageBox::warning(this, NULL, messageToDisplay);
                }
                conflict = true;

                sourcesAndDestinations.removeAt(j);
                j--; //one step behind since the List lost 1 element
                //no problem about the (i == j) check, since we are moving the good way
            }
        }

        //if this "i" got conflicts, we remove it from the move process too.
        if (conflict == true)
        {
            sourcesAndDestinations.removeAt(i);
            i--; //one step behind since the List lost 1 element
        }
    }

    //5 - We got to move it
    for (auto currentMove : sourcesAndDestinations)
    {
        bool errorFlag = false; //Can become true after some tests.

        QString currentFile = currentMove[0];
        QString restoredFilePath = currentMove[1];

        QFile destination(restoredFilePath);
        bool bOverwrite = ui->checkBox_overwrite->isChecked();

        //5.1 - Check if the destination is already taken or not
        if (destination.exists() == true)
        {
            //Do we have to overwrite ?
            if (bOverwrite == true)
            {
                bool bRet = destination.remove();
                if (bRet == false)
                {
                    errorFlag = true;
                    QString messageToDisplay = "Unable to overwrite " + restoredFilePath + "\n\n";
                    messageToDisplay += "May be the destination file is locked by another task ? "
                                        "Disk space ? Permissions ?";
                    QMessageBox::warning(this, NULL, messageToDisplay);
                }
            }
            else
            {
                errorFlag = true;
                QString messageToDisplay = "Won't overwrite " + restoredFilePath;
                QMessageBox::information(this, NULL, messageToDisplay);
            }
        }

        //5.2 - If no error, let's finally move it for real
        if (errorFlag == false)
        {
            bool bRet = handyDir.rename(currentFile, restoredFilePath);
            if (bRet == false)
            {
                QString messageToDisplay = "Cannot move :\n" + currentFile + "\nto :\n" + restoredFilePath + "\n\n";
                messageToDisplay += "Disk space ? Permissions ? Source locked by another task ?";
                QMessageBox::warning(this, NULL, messageToDisplay);
            }
        }
    }

    //5 - Tells the user we have finished
    QMessageBox::information(this, NULL, "Work terminated - check everything into your destination and into what remains into your .stversions folder.");
}

void MainWindow::on_checkBox_restoreAll_clicked(bool checked)
{
    ui->dateTimeEdit_from->setDisabled(checked);
    ui->dateTimeEdit_to->setDisabled(checked);
}




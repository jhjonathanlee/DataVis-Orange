#include <QTextDocument>
#include <QTextCursor>
#include <QTextTable>
#include <QDesktopWidget>
#include <QRect>
#include <QPrinter>
#include <QPainter>
#include <QPrintDialog>
#include <QFileDialog>
#include <QtGlobal>
#include <QList>
#include <QString>
#include <QMessageBox>
#include <stdexcept>

#include "FileInputDialog.h"
#include "DashboardWindow.h"
#include "PresentationDashboardWindow.h"
#include "TeachingDashboardWindow.h"
#include "GrantDashboardWindow.h"
#include "PublicationDashboardWindow.h"
#include "ui_DashboardWindow.h"
#include "../parser/PresentationParser.h"
#include "../parser/GrantParser.h"
#include "../parser/PublicationParser.h"
#include "../parser/TeachingParser.h"
#include "../records/PresentationRecord.h"
#include "../records/GrantRecord.h"
#include "../records/PublicationRecord.h"
#include "../records/TeachingRecord.h"

DashboardWindow::DashboardWindow() {
    ui.setupUi(this);
	
	//set size to 70% of total screen size
    QDesktopWidget dw;
    QRect mainScreen = dw.availableGeometry(dw.primaryScreen());
    this->resize(mainScreen.width()*0.7, mainScreen.height()*0.7);
}

DashboardWindow *DashboardWindow::makeDashboard() {
	FileInputDialog inputDialog;

    PresentationParser presParser;
    GrantParser grantParser;
    PublicationParser pubParser;
    TeachingParser teachParser;

    while (inputDialog.exec() == QDialog::Accepted) {

        QString csv = inputDialog.getFilename();

		if (inputDialog.getSubjectArea() == Presentation) {
            QList<PresentationRecord> records;

            try {
                records = presParser.parse(csv);
            } catch (const std::exception &e) {
                //qDebug() << e.what();
                QMessageBox::critical(NULL, "Error", "A fatal error occurred while parsing the CSV file");
                continue;
            }
            return new PresentationDashboardWindow(records, csv);
        }
        else if (inputDialog.getSubjectArea() == Teaching) {
            QList<TeachingRecord> records;

            try {
                records = teachParser.parse(csv);
            } catch (const std::exception &e) {
                //qDebug() << e.what();
                QMessageBox::critical(NULL, "Error", "A fatal error occurred while parsing the CSV file");
                continue;
            }
            return new TeachingDashboardWindow(records, csv);
        }
        else if (inputDialog.getSubjectArea() == Grants) {
            QList<GrantRecord> records;

            try {
                records = grantParser.parse(csv);
            } catch (const std::exception &e) {
                //qDebug() << e.what();
                QMessageBox::critical(NULL, "Error", "A fatal error occurred while parsing the CSV file");
                continue;
            }
            return new GrantDashboardWindow(records, csv);
        }
        else if (inputDialog.getSubjectArea() == Publications) {
            QList<PublicationRecord> records;

            try {
                records = pubParser.parse(csv);
            } catch (const std::exception &e) {
                //qDebug() << e.what();
                QMessageBox::critical(NULL, "Error", "A fatal error occurred while parsing the CSV file");
                continue;
            }
            return new PublicationDashboardWindow(records, csv);
        }
        else {
			throw std::invalid_argument("Unknown subject area");
		}
    }

    return nullptr;
}

void DashboardWindow::on_dateFilterButton_clicked() {
	updateDateLabel();
	updateTreeWidget();
}

void DashboardWindow::on_actionOpen_triggered() {
    DashboardWindow *dashboard = DashboardWindow::makeDashboard();
    if (dashboard != nullptr)
        dashboard->show();

}

void DashboardWindow::updateDateLabel() {
	ui.dateRangeLabel->setText("Showing records from " + 
							   ui.startDateSelector->date().toString("MMM d yyyy") + 
							   " to " + 
							   ui.endDateSelector->date().toString("MMM d yyyy"));
}

void DashboardWindow::on_actionClose_triggered() {
	close();
}

void DashboardWindow::setColumnWidths() {
    // for now, make sure that column width is at least equal to contents
    for (int i = 0; i < ui.treeWidget->columnCount(); i++) {
        ui.treeWidget->resizeColumnToContents(i);
    }
}

void DashboardWindow::on_treeWidget_collapsed() {
    setColumnWidths();
}

void DashboardWindow::on_treeWidget_expanded() {
    setColumnWidths();
}

int treeWidgetToTextTable(QTreeWidgetItem *item, QTextTable *table, int rowNum) {
	for (int c = 0; c < item->childCount(); ++c) {
		QTreeWidgetItem *child = item->child(c);
		
		//export child
		table->appendRows(1);
		for (int colNum = 0; colNum < child->columnCount(); ++colNum) {
			table->cellAt(rowNum, colNum).firstCursorPosition().insertText(child->text(colNum));
		}
		++rowNum;
		
		//recurse to children
		if (child->isExpanded() && child->childCount() > 0) {
			rowNum = treeWidgetToTextTable(child, table, rowNum);
		}
	}
	
	return rowNum;
}

void DashboardWindow::printTreeWidget(QPrinter *printer) {
	//build a table
    QTextDocument doc;
	QTextCursor cursor(&doc);
	QTextTable *table = cursor.insertTable(1, ui.treeWidget->columnCount());
	
	//header
	for (int colNum = 0; colNum < ui.treeWidget->columnCount(); ++colNum) {
		table->cellAt(0, colNum).firstCursorPosition().insertText(ui.treeWidget->headerItem()->text(colNum));
	}
	
	//table content
	treeWidgetToTextTable(ui.treeWidget->invisibleRootItem(), table, 1);
	
	doc.print(printer);
}

void DashboardWindow::on_actionPrint_triggered() {
    // set up the printer
    QPrinter printer;

    QPrintDialog dialog(&printer, this);
    dialog.setWindowTitle("Print Document");

    if (dialog.exec() != QDialog::Accepted)
        return;

    printTreeWidget(&printer);
}

void DashboardWindow::on_actionExport_triggered() {
    // set up the printer
	QString filename = QFileDialog::getSaveFileName(this, "Save Dashboard", "", "PDF (*.pdf)");
	if (filename.isEmpty())
		return;
	
    QPrinter printer(QPrinter::HighResolution);
	printer.setOutputFormat(QPrinter::PdfFormat);
	printer.setOutputFileName(filename);

    printTreeWidget(&printer);
}

void DashboardWindow::on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item) {
    QString memberName = item->text(memberNameColumn());
	if (!memberName.isEmpty())
		openVisualizationWindow(memberName);
}

void DashboardWindow::on_openVisualizationButton_clicked() {
	QString memberName = ui.visualizationFacultyNameSelector->currentText();
	if (!memberName.isEmpty())
		openVisualizationWindow(memberName);
}

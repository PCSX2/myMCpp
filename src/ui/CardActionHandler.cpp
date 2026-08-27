// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "CardActionHandler.h"
#include "PS2MemoryCard.h"
#include "PS2SaveFile.h"
#include "QtUtils.h"
#include <QMessageBox>
#include <QStatusBar>
#include <QFileInfo>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QPushButton>

CardActionHandler::CardActionHandler(QWidget* parent)
	: QObject(parent)
	, parentWidget(parent)
	, statusBar(nullptr)
{
}

PS2MemoryCard* CardActionHandler::openCard(const QString& filename)
{
	auto card = std::make_unique<PS2MemoryCard>();
	if (!card->open(filename.toStdString()))
	{
		QMessageBox::critical(parentWidget, tr("Error"),
			tr("Failed to open memory card: %1").arg(QString::fromStdString(card->GetError().GetDescription())));
		return nullptr;
	}

	if (statusBar)
	{
		statusBar->showMessage(tr("Opened: %1").arg(filename), 3000);
	}

	return card.release();
}

PS2MemoryCard* CardActionHandler::createCard(const QString& filename, int sizeMB, bool disableEcc)
{
	auto card = std::make_unique<PS2MemoryCard>();
	if (!card->create(filename.toStdString(), sizeMB, disableEcc))
	{
		QMessageBox::critical(parentWidget, tr("Error"),
			tr("Failed to create memory card: %1").arg(QString::fromStdString(card->GetError().GetDescription())));
		return nullptr;
	}

	if (statusBar)
	{
		statusBar->showMessage(tr("Created: %1").arg(filename), 3000);
	}

	return card.release();
}

void CardActionHandler::closeCard()
{
	if (statusBar)
	{
		statusBar->showMessage(tr("No memory card open"));
	}
}


CardActionHandler::ImportResult CardActionHandler::importSave(PS2MemoryCard* card, const std::shared_ptr<PS2SaveFile>& saveFile, const QString& /*filename*/, bool showDialogs, bool forceOverwrite)
{
	if (!card)
	{
		if (showDialogs)
		{
			QMessageBox::warning(parentWidget, tr("Warning"), tr("No memory card open"));
		}
		return ImportResult::Failed;
	}

	if (!saveFile)
	{
		return ImportResult::Failed;
	}

	const auto& entries = saveFile->getEntries();
	if (entries.empty())
	{
		if (showDialogs)
		{
			QMessageBox::warning(parentWidget, tr("Warning"),
				tr("Save file is empty or contains no valid entries"));
		}
		return ImportResult::Failed;
	}

	const bool hasDirHeader = !entries.empty() && (entries[0].dirEntry.mode & DF_DIR);
	std::string saveName = hasDirHeader ? entries[0].dirEntry.name : saveFile->getTitle();
	if (saveName.empty())
	{
		saveName = entries[0].dirEntry.name;
	}

	bool result = card->importSaveFile(*saveFile, forceOverwrite, "");

	if (result)
	{
		if (showDialogs)
		{
			QMessageBox::information(parentWidget, tr("Success"),
				tr("Successfully imported save: %1")
					.arg(QString::fromStdString(saveName)));
		}

		if (statusBar)
		{
			statusBar->showMessage(tr("Imported: %1").arg(QString::fromStdString(saveName)), 3000);
		}
		return ImportResult::Success;
	}
	else
	{
		if (card->GetError().IsValid())
		{
			if (showDialogs)
			{
				QMessageBox::critical(parentWidget, tr("Error"),
					tr("Failed to import save: %1").arg(QString::fromStdString(card->GetError().GetDescription())));
			}
			return ImportResult::Failed;
		}

		if (!showDialogs)
		{
			return ImportResult::Failed;
		}

		auto reply = QMessageBox::question(parentWidget, tr("Save Exists"),
			tr("Save '%1' already exists. Overwrite?")
				.arg(QString::fromStdString(saveName)),
			QMessageBox::Yes | QMessageBox::No);

		if (reply == QMessageBox::Yes)
		{
			result = card->importSaveFile(*saveFile, true, "");

			if (result)
			{
				QMessageBox::information(parentWidget, tr("Success"),
					tr("Successfully imported and overwrote existing save"));

				if (statusBar)
				{
					statusBar->showMessage(tr("Imported (overwrite): %1")
											   .arg(QString::fromStdString(saveName)),
						3000);
				}
				return ImportResult::Success;
			}
			else
			{
				if (showDialogs && card->GetError().IsValid())
				{
					QMessageBox::critical(parentWidget, tr("Error"),
						tr("Failed to import save: %1").arg(QString::fromStdString(card->GetError().GetDescription())));
				}
				return ImportResult::Failed;
			}
		}
		else
		{
			return ImportResult::Skipped;
		}
	}
}

bool CardActionHandler::exportSave(PS2MemoryCard* card, const QString& savePath, const QString& outputFilename, bool showSuccessDialog)
{
	if (!card)
	{
		if (showSuccessDialog)
			QMessageBox::warning(parentWidget, tr("Warning"), tr("No memory card open"));
		return false;
	}

	PS2SaveFile saveFile;
	if (!card->exportSaveFile(savePath.toStdString(), saveFile))
	{
		if (showSuccessDialog)
			QMessageBox::critical(parentWidget, tr("Error"),
				tr("Failed to export save: %1").arg(QString::fromStdString(card->GetError().GetDescription())));
		return false;
	}

	SaveFormat format = PS2SaveFile::detectFormat(outputFilename.toStdString());
	if (format == SaveFormat::UNKNOWN)
		format = SaveFormat::EMS;

	if (!saveFile.save(outputFilename.toStdString(), format))
	{
		if (showSuccessDialog)
			QMessageBox::critical(parentWidget, tr("Error"),
				tr("Failed to export save: %1").arg(QString::fromStdString(saveFile.GetError().GetDescription())));
		return false;
	}

	if (showSuccessDialog)
	{
		QMessageBox msgBox(parentWidget);
		msgBox.setWindowTitle(tr("Success"));
		msgBox.setText(tr("Successfully exported save to:\n%1").arg(outputFilename));
		msgBox.setIcon(QMessageBox::Information);
		QPushButton* openFolderButton = msgBox.addButton(tr("Open Folder"), QMessageBox::ActionRole);
		msgBox.addButton(QMessageBox::Ok);
		msgBox.exec();

		if (msgBox.clickedButton() == openFolderButton)
		{
			QString folderPath = QFileInfo(outputFilename).absolutePath();
			QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
		}
	}
	if (statusBar && showSuccessDialog)
		statusBar->showMessage(tr("Exported: %1").arg(savePath), 3000);
	return true;
}

bool CardActionHandler::exportSaveAsFolder(PS2MemoryCard* card, const QString& savePath, const QString& targetDir, bool showSuccessDialog)
{
	if (!card)
	{
		if (showSuccessDialog)
			QMessageBox::warning(parentWidget, tr("Warning"), tr("No memory card open"));
		return false;
	}

	PS2SaveFile saveFile;
	if (!card->exportSaveFile(savePath.toStdString(), saveFile))
	{
		if (showSuccessDialog)
			QMessageBox::critical(parentWidget, tr("Error"),
				tr("Failed to export save: %1").arg(QString::fromStdString(card->GetError().GetDescription())));
		return false;
	}

	QStringList sanitizedChanges;
	QString rawFolderName = QFileInfo(savePath).fileName();
	QString saveFolderName = QtUtils::sanitizeFilename(rawFolderName);
	if (saveFolderName != rawFolderName)
	{
		sanitizedChanges.append(tr("'%1' -> '%2'").arg(rawFolderName, saveFolderName));
	}
	QDir destParentDir(targetDir);
	QString fullDestPath = destParentDir.filePath(saveFolderName);

	QDir destDir(fullDestPath);
	if (!destDir.exists())
	{
		if (!destParentDir.mkpath(saveFolderName))
		{
			if (showSuccessDialog)
				QMessageBox::critical(parentWidget, tr("Error"),
					tr("Failed to create directory: %1").arg(fullDestPath));
			return false;
		}
	}

	const auto& entries = saveFile.getEntries();
	for (const auto& entry : entries)
	{
		if (entry.dirEntry.mode & DF_DIR)
		{
			continue;
		}

		QString rawFileName = QString::fromStdString(entry.dirEntry.name);
		QString fileName = QtUtils::sanitizeFilename(rawFileName);
		if (fileName != rawFileName)
		{
			sanitizedChanges.append(tr("'%1' -> '%2'").arg(rawFileName, fileName));
		}
		QString filePath = destDir.filePath(fileName);

		QFile file(filePath);
		if (!file.open(QIODevice::WriteOnly))
		{
			if (showSuccessDialog)
				QMessageBox::critical(parentWidget, tr("Error"),
					tr("Failed to write file: %1").arg(filePath));
			return false;
		}
		if (file.write(reinterpret_cast<const char*>(entry.data.data()), entry.data.size()) != static_cast<qint64>(entry.data.size()))
		{
			if (showSuccessDialog)
				QMessageBox::critical(parentWidget, tr("Error"),
					tr("Failed to write all data to file: %1").arg(filePath));
			return false;
		}
	}

	if (showSuccessDialog)
	{
		QMessageBox msgBox(parentWidget);
		msgBox.setWindowTitle(tr("Success"));
		QString msgText = tr("Successfully exported save as folder to:\n%1").arg(fullDestPath);
		if (!sanitizedChanges.isEmpty())
		{
			msgText += "\n\n" + tr("Note: The following names were sanitized due to invalid characters:\n%1").arg(sanitizedChanges.join("\n"));
		}
		msgBox.setText(msgText);
		msgBox.setIcon(QMessageBox::Information);
		QPushButton* openFolderButton = msgBox.addButton(tr("Open Folder"), QMessageBox::ActionRole);
		msgBox.addButton(QMessageBox::Ok);
		msgBox.exec();

		if (msgBox.clickedButton() == openFolderButton)
		{
			QDesktopServices::openUrl(QUrl::fromLocalFile(fullDestPath));
		}
	}
	if (statusBar && showSuccessDialog)
		statusBar->showMessage(tr("Exported: %1").arg(savePath), 3000);
	return true;
}

bool CardActionHandler::deleteSave(PS2MemoryCard* card, const QString& savePath, bool showDialogs)
{
	if (!card)
	{
		if (showDialogs)
			QMessageBox::warning(parentWidget, tr("Warning"), tr("No memory card open"));
		return false;
	}

	QString saveName = savePath;
	if (saveName.startsWith("/"))
	{
		saveName = saveName.mid(1);
	}

	if (!card->remove(savePath.toStdString()))
	{
		if (showDialogs)
		{
			QMessageBox::critical(parentWidget, tr("Error"),
				tr("Failed to delete save:\n\n%1").arg(QString::fromStdString(card->GetError().GetDescription())));
		}
		return false;
	}

	if (showDialogs && statusBar)
	{
		statusBar->showMessage(tr("Deleted: %1").arg(saveName), 3000);
	}
	return true;
}

void CardActionHandler::formatCard(PS2MemoryCard* card, const QString& cardPath, int sizeMB)
{
	if (!card)
	{
		QMessageBox::warning(parentWidget, tr("Warning"), tr("No memory card open"));
		return;
	}

	int effectiveSizeMB = sizeMB;

	int ret = QMessageBox::warning(parentWidget, tr("Format Card"),
		tr("WARNING: This will erase ALL data on the memory card!\n\n"
		   "Are you sure you want to continue?"),
		QMessageBox::Yes | QMessageBox::No,
		QMessageBox::No);

	if (ret == QMessageBox::Yes)
	{
		if (effectiveSizeMB <= 0)
		{
			const auto info = card->getCardInfo();
			if (card->GetError().IsValid())
			{
				QMessageBox::critical(parentWidget, tr("Error"),
					tr("Failed to read card info:\n\n%1").arg(QString::fromStdString(card->GetError().GetDescription())));
				return;
			}
			effectiveSizeMB = static_cast<int>(info.imageSizeBytes / (1024ull * 1024ull));
			if (effectiveSizeMB <= 0)
			{
				QMessageBox::critical(parentWidget, tr("Error"),
					tr("Failed to format card:\n\nInvalid memory card size"));
				return;
			}
		}

		const bool disableEcc = !card->hasEcc();

		card->close();
		if (!card->create(cardPath.toStdString(), effectiveSizeMB, disableEcc))
		{
			QMessageBox::critical(parentWidget, tr("Error"),
				tr("Failed to format card:\n\n%1").arg(QString::fromStdString(card->GetError().GetDescription())));
			return;
		}

		if (statusBar)
		{
			statusBar->showMessage(tr("Card formatted successfully"), 3000);
		}

		QMessageBox::information(parentWidget, tr("Success"),
			tr("Memory card formatted successfully (%1 MB)").arg(effectiveSizeMB));
	}
}

void CardActionHandler::setStatusBar(QStatusBar* bar)
{
	statusBar = bar;
}

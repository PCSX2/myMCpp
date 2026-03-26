// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "CardActionHandler.h"
#include "ps2mc.h"
#include "ps2save.h"
#include <QMessageBox>
#include <QStatusBar>
#include <QFileInfo>

CardActionHandler::CardActionHandler(QWidget* parent)
	: QObject(parent)
	, parentWidget(parent)
	, statusBar(nullptr)
{
}

PS2MemoryCard* CardActionHandler::openCard(const QString& filename)
{
	try
	{
		auto card = std::make_unique<PS2MemoryCard>();
		card->open(filename.toStdString());

		if (statusBar)
		{
			statusBar->showMessage(tr("Opened: %1").arg(filename), 3000);
		}

		return card.release();
	}
	catch (const std::exception& e)
	{
		QMessageBox::critical(parentWidget, tr("Error"),
			tr("Failed to open memory card: %1").arg(e.what()));
		return nullptr;
	}
}

PS2MemoryCard* CardActionHandler::createCard(const QString& filename, int sizeMB, bool disableEcc)
{
	try
	{
		auto card = std::make_unique<PS2MemoryCard>();
		card->create(filename.toStdString(), sizeMB, disableEcc);

		if (statusBar)
		{
			statusBar->showMessage(tr("Created: %1").arg(filename), 3000);
		}

		return card.release();
	}
	catch (const std::exception& e)
	{
		QMessageBox::critical(parentWidget, tr("Error"),
			tr("Failed to create memory card: %1").arg(e.what()));
		return nullptr;
	}
}

void CardActionHandler::closeCard()
{
	if (statusBar)
	{
		statusBar->showMessage(tr("No memory card open"));
	}
}

void CardActionHandler::importSave(PS2MemoryCard* card, const QString& filename)
{
	if (!card)
	{
		QMessageBox::warning(parentWidget, tr("Warning"), tr("No memory card open"));
		return;
	}

	if (!QFileInfo(filename).exists())
	{
		QMessageBox::critical(parentWidget, tr("Error"),
			tr("File not found: %1").arg(filename));
		return;
	}

	try
	{
		PS2SaveFile saveFile;
		saveFile.load(filename.toStdString());

		const auto& entries = saveFile.getEntries();
		if (entries.empty())
		{
			QMessageBox::warning(parentWidget, tr("Warning"),
				tr("Save file is empty or contains no valid entries"));
			return;
		}

		std::string saveName = entries[0].dirEntry.name;

		bool result = card->importSaveFile(saveFile, false, "");

		if (result)
		{
			QMessageBox::information(parentWidget, tr("Success"),
				tr("Successfully imported save: %1")
					.arg(QString::fromStdString(saveName)));

			if (statusBar)
			{
				statusBar->showMessage(tr("Imported: %1").arg(QString::fromStdString(saveName)), 3000);
			}
		}
		else
		{
			auto reply = QMessageBox::question(parentWidget, tr("Save Exists"),
				tr("Save '%1' already exists. Overwrite?")
					.arg(QString::fromStdString(saveName)),
				QMessageBox::Yes | QMessageBox::No);

			if (reply == QMessageBox::Yes)
			{
				result = card->importSaveFile(saveFile, true, "");

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
				}
			}
		}
	}
	catch (const std::exception& e)
	{
		QMessageBox::critical(parentWidget, tr("Error"),
			tr("Failed to import save: %1").arg(e.what()));
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

	try
	{
		PS2SaveFile saveFile;
		card->exportSaveFile(savePath.toStdString(), saveFile);

		SaveFormat format = PS2SaveFile::detectFormat(outputFilename.toStdString());
		if (format == SaveFormat::UNKNOWN)
			format = SaveFormat::EMS;

		saveFile.save(outputFilename.toStdString(), format);

		if (showSuccessDialog)
		{
			QMessageBox::information(parentWidget, tr("Success"),
				tr("Successfully exported save to:\n%1").arg(outputFilename));
		}
		if (statusBar && showSuccessDialog)
			statusBar->showMessage(tr("Exported: %1").arg(savePath), 3000);
		return true;
	}
	catch (const std::exception& e)
	{
		if (showSuccessDialog)
			QMessageBox::critical(parentWidget, tr("Error"), tr("Failed to export save: %1").arg(e.what()));
		return false;
	}
}

void CardActionHandler::deleteSave(PS2MemoryCard* card, const QString& savePath)
{
	if (!card)
	{
		QMessageBox::warning(parentWidget, tr("Warning"), tr("No memory card open"));
		return;
	}

	QString saveName = savePath;
	if (saveName.startsWith("/"))
	{
		saveName = saveName.mid(1);
	}

	int ret = QMessageBox::question(parentWidget, tr("Confirm Delete"),
		tr("Delete this save?\n\n%1").arg(saveName),
		QMessageBox::Yes | QMessageBox::No);

	if (ret == QMessageBox::Yes)
	{
		try
		{
			card->remove(savePath.toStdString());

			if (statusBar)
			{
				statusBar->showMessage(tr("Deleted: %1").arg(saveName), 3000);
			}
		}
		catch (const std::exception& e)
		{
			QMessageBox::critical(parentWidget, tr("Error"),
				tr("Failed to delete save:\n\n%1").arg(e.what()));
		}
	}
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
		try
		{
			if (effectiveSizeMB <= 0)
			{
				const auto info = card->getCardInfo();
				effectiveSizeMB = static_cast<int>(info.imageSizeBytes / (1024ull * 1024ull));
				if (effectiveSizeMB <= 0)
				{
					throw std::runtime_error("Invalid memory card size");
				}
			}

			const bool disableEcc = !card->hasEcc();

			card->close();
			card->create(cardPath.toStdString(), effectiveSizeMB, disableEcc);

			if (statusBar)
			{
				statusBar->showMessage(tr("Card formatted successfully"), 3000);
			}

			QMessageBox::information(parentWidget, tr("Success"),
				tr("Memory card formatted successfully (%1 MB)").arg(effectiveSizeMB));
		}
		catch (const std::exception& e)
		{
			QMessageBox::critical(parentWidget, tr("Error"),
				tr("Failed to format card:\n\n%1").arg(e.what()));
		}
	}
}

void CardActionHandler::setStatusBar(QStatusBar* bar)
{
	statusBar = bar;
}

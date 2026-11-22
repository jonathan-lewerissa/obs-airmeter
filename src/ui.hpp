/*
obs-airmeter
Copyright (C) 2026 Jonathan Lewerissa <jonathan.rehuel16@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#pragma once

#include <QDialog>
#include <QFrame>
#include <QPointer>
#include <QTimer>
#include <QWidget>

#include <cmath>
#include <string>
#include <vector>

#include "history.hpp"
#include "presets.h"

class QComboBox;
class QDoubleSpinBox;
class QLabel;

struct AirmeterSettings {
	int track = 0;
	std::string preset = "youtube";
	double custom_target = -14.0;
	double ceiling = AIRMETER_DEFAULT_CEILING_DBTP;
	int layout = 0;

	double target() const;
	QString presetLabel() const;

	static AirmeterSettings load();
	void save() const;
};

class MeterModel : public QObject {
	Q_OBJECT

public:
	explicit MeterModel(QObject *parent);

	AirmeterSettings settings;
	loudness_values now{NAN, NAN, NAN, NAN, NAN, NAN};
	History history;

	void reset();
	void apply(const AirmeterSettings &next);

signals:
	void updated();
	void settingsChanged();

private:
	QTimer timer;
};

class ValueTile : public QFrame {
public:
	ValueTile(const QString &label, const QString &unit, QWidget *parent = nullptr);
	void setValue(double v, const QColor &color);

private:
	QLabel *number;
};

class HistoryGraph : public QWidget {
public:
	struct Series {
		std::vector<double> values;
		size_t capacity;
		QColor color;
		qreal width;
	};

	HistoryGraph(const QString &title, double y_min, double y_max, QWidget *parent = nullptr);

	void setSeries(std::vector<Series> s);
	void setBand(double low, double high);
	void setCeiling(double value);

protected:
	void paintEvent(QPaintEvent *) override;

private:
	QString title;
	double y_min, y_max;
	double band_low = NAN, band_high = NAN, ceiling = NAN;
	std::vector<Series> series;
};

class LoudnessBar : public QWidget {
public:
	explicit LoudnessBar(QWidget *parent = nullptr);
	void setValue(double v, double target, const QColor &color);

protected:
	void paintEvent(QPaintEvent *) override;

private:
	double value = NAN, target = NAN;
	QColor color;
};

class AirmeterWindow : public QWidget {
public:
	explicit AirmeterWindow(MeterModel *model);

protected:
	void closeEvent(QCloseEvent *event) override;

private:
	void build();
	void refresh();

	MeterModel *model;
	int built_layout = -1;
	QWidget *content = nullptr;
	QLabel *header = nullptr;
	ValueTile *tile_s = nullptr, *tile_m = nullptr, *tile_i = nullptr, *tile_lra = nullptr, *tile_tp = nullptr;
	HistoryGraph *graph_sm = nullptr, *graph_s = nullptr, *graph_i = nullptr, *graph_tp = nullptr;
};

class AirmeterDock : public QWidget {
public:
	explicit AirmeterDock(QWidget *parent = nullptr);

	MeterModel *model;
	QPointer<AirmeterWindow> window;

private:
	void refresh();
	void openWindow();

	QLabel *track, *s_value, *status, *m_value, *i_value, *tp_value;
	LoudnessBar *bar;
};

class SettingsDialog : public QDialog {
public:
	SettingsDialog(MeterModel *model, QWidget *parent);

private:
	void save();

	MeterModel *model;
	QComboBox *track, *preset, *layout;
	QDoubleSpinBox *custom_target, *ceiling;
};

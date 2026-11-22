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

#include "ui.hpp"

#include <QCloseEvent>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <util/config-file.h>

#include "meter.h"

static const char *const section = "obs-airmeter";

static const QColor green(0x5b, 0xd1, 0x8b);
static const QColor amber(0xf0, 0xb4, 0x4c);
static const QColor red(0xef, 0x5a, 0x5a);
static const QColor blue(0x4c, 0x8b, 0xf5);

static QString T(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

static QString fmt(double v)
{
	return std::isnan(v) ? T("Value.None") : QString::number(v, 'f', 1);
}

static QColor neutral(const QWidget *w)
{
	return w->palette().color(QPalette::WindowText);
}

static QColor statusColor(enum airmeter_status st, const QWidget *w)
{
	switch (st) {
	case AIRMETER_ON_TARGET:
		return green;
	case AIRMETER_BIT_LOUD:
	case AIRMETER_BIT_QUIET:
		return amber;
	case AIRMETER_TOO_LOUD:
	case AIRMETER_TOO_QUIET:
		return red;
	default:
		return w->palette().color(QPalette::Disabled, QPalette::WindowText);
	}
}

static QString statusText(enum airmeter_status st)
{
	static const char *const keys[] = {"Status.NoSignal", "Status.OnTarget", "Status.BitLoud",
					   "Status.TooLoud",  "Status.BitQuiet", "Status.TooQuiet"};
	return T(keys[st]);
}

static QColor peakColor(double tp, double ceiling, const QWidget *w)
{
	if (std::isnan(tp))
		return statusColor(AIRMETER_NO_SIGNAL, w);
	return tp > ceiling ? red : green;
}

static void setColor(QLabel *label, const QColor &c)
{
	label->setStyleSheet(QStringLiteral("color: %1").arg(c.name()));
}

static void scaleFont(QWidget *w, qreal factor, bool bold)
{
	QFont f = w->font();
	f.setPointSizeF(f.pointSizeF() * factor);
	f.setBold(bold);
	w->setFont(f);
}

static QString trackText(int track)
{
	return T("Track").arg(track + 1);
}

double AirmeterSettings::target() const
{
	const airmeter_preset *p = airmeter_find_preset(preset.c_str());
	return std::isnan(p->target) ? custom_target : p->target;
}

QString AirmeterSettings::presetLabel() const
{
	return QStringLiteral("%1 (%2 %3)")
		.arg(T(airmeter_find_preset(preset.c_str())->text_key), fmt(target()), T("Unit.LUFS"));
}

AirmeterSettings AirmeterSettings::load()
{
	AirmeterSettings s;
	config_t *c = obs_frontend_get_user_config();
	if (!c)
		return s;

	config_set_default_int(c, section, "Track", s.track);
	config_set_default_string(c, section, "Preset", s.preset.c_str());
	config_set_default_double(c, section, "CustomTarget", s.custom_target);
	config_set_default_double(c, section, "Ceiling", s.ceiling);
	config_set_default_int(c, section, "Layout", s.layout);

	s.track = (int)std::clamp<int64_t>(config_get_int(c, section, "Track"), 0, 5);
	s.preset = airmeter_find_preset(config_get_string(c, section, "Preset"))->id;
	s.custom_target = std::clamp(config_get_double(c, section, "CustomTarget"), -30.0, -5.0);
	s.ceiling = std::clamp(config_get_double(c, section, "Ceiling"), -3.0, 0.0);
	s.layout = config_get_int(c, section, "Layout") == 1 ? 1 : 0;
	return s;
}

void AirmeterSettings::save() const
{
	config_t *c = obs_frontend_get_user_config();
	if (!c)
		return;
	config_set_int(c, section, "Track", track);
	config_set_string(c, section, "Preset", preset.c_str());
	config_set_double(c, section, "CustomTarget", custom_target);
	config_set_double(c, section, "Ceiling", ceiling);
	config_set_int(c, section, "Layout", layout);
	config_save_safe(c, "tmp", nullptr);
}

MeterModel::MeterModel(QObject *parent) : QObject(parent), settings(AirmeterSettings::load())
{
	timer.setInterval(100);
	connect(&timer, &QTimer::timeout, this, [this] {
		meter_read(&now);
		history.add(now);
		emit updated();
	});
	timer.start();
}

void MeterModel::reset()
{
	meter_reset();
	history.clear();
	emit updated();
}

void MeterModel::apply(const AirmeterSettings &next)
{
	bool track_changed = next.track != settings.track;
	settings = next;
	settings.save();
	if (track_changed) {
		meter_set_mix((size_t)settings.track);
		history.clear();
	}
	emit settingsChanged();
}

ValueTile::ValueTile(const QString &label, const QString &unit, QWidget *parent) : QFrame(parent)
{
	setFrameShape(QFrame::StyledPanel);
	auto *name = new QLabel(label);
	number = new QLabel(T("Value.None"));
	auto *unit_label = new QLabel(unit);
	scaleFont(number, 2.0, true);
	number->setMinimumWidth(number->fontMetrics().horizontalAdvance(QStringLiteral("-00.0")));

	auto *box = new QVBoxLayout(this);
	box->addWidget(name);
	box->addWidget(number);
	box->addWidget(unit_label);
}

void ValueTile::setValue(double v, const QColor &color)
{
	number->setText(fmt(v));
	setColor(number, color);
}

HistoryGraph::HistoryGraph(const QString &title_, double y_min_, double y_max_, QWidget *parent)
	: QWidget(parent),
	  title(title_),
	  y_min(y_min_),
	  y_max(y_max_)
{
	setMinimumSize(200, 100);
}

void HistoryGraph::setSeries(std::vector<Series> s)
{
	series = std::move(s);
	update();
}

void HistoryGraph::setBand(double low, double high)
{
	band_low = low;
	band_high = high;
}

void HistoryGraph::setCeiling(double value)
{
	ceiling = value;
}

void HistoryGraph::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);
	QFontMetrics fm(font());
	QRectF r = QRectF(rect()).adjusted(fm.horizontalAdvance(QStringLiteral("-00")) + 8, fm.height() + 4, -4, -4);
	auto y = [&](double v) {
		return r.bottom() - (std::clamp(v, y_min, y_max) - y_min) / (y_max - y_min) * r.height();
	};

	p.fillRect(r, palette().color(QPalette::Base));
	p.setPen(neutral(this));
	p.drawText(QPointF(r.left(), fm.ascent()), title);

	for (double g = std::ceil(y_min / 10) * 10; g <= y_max; g += 10) {
		p.setPen(palette().color(QPalette::Mid));
		p.drawLine(QPointF(r.left(), y(g)), QPointF(r.right(), y(g)));
		p.setPen(neutral(this));
		p.drawText(QRectF(0, y(g) - fm.height() / 2.0, r.left() - 4, fm.height()),
			   Qt::AlignRight | Qt::AlignVCenter, QString::number(g));
	}

	if (!std::isnan(band_low)) {
		QColor band = green;
		band.setAlpha(60);
		p.fillRect(QRectF(QPointF(r.left(), y(band_high)), QPointF(r.right(), y(band_low))), band);
	}
	if (!std::isnan(ceiling)) {
		p.setPen(QPen(red, 1, Qt::DashLine));
		p.drawLine(QPointF(r.left(), y(ceiling)), QPointF(r.right(), y(ceiling)));
	}

	for (const Series &s : series) {
		/* If capacity is 0, the series uses the full width for all values. */
		size_t cap = s.capacity ? s.capacity : s.values.size();
		if (cap < 2)
			continue;
		double dx = r.width() / double(cap - 1);
		double x0 = r.left() + double(cap - s.values.size()) * dx;
		p.setPen(QPen(s.color, s.width));
		QPolygonF line;
		for (size_t k = 0; k < s.values.size(); k++) {
			if (std::isnan(s.values[k])) {
				if (line.size() > 1)
					p.drawPolyline(line);
				line.clear();
				continue;
			}
			line << QPointF(x0 + double(k) * dx, y(s.values[k]));
		}
		if (line.size() > 1)
			p.drawPolyline(line);
	}
}

static constexpr double bar_min = -40.0;
static constexpr double bar_max = 0.0;

LoudnessBar::LoudnessBar(QWidget *parent) : QWidget(parent)
{
	setMinimumHeight(14);
}

void LoudnessBar::setValue(double v, double t, const QColor &c)
{
	value = v;
	target = t;
	color = c;
	update();
}

void LoudnessBar::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	QRectF r = QRectF(rect()).adjusted(0, 3, 0, -3);
	auto x = [&](double v) {
		return r.left() + (std::clamp(v, bar_min, bar_max) - bar_min) / (bar_max - bar_min) * r.width();
	};

	p.fillRect(r, palette().color(QPalette::Base));
	if (!std::isnan(value))
		p.fillRect(QRectF(QPointF(r.left(), r.top()), QPointF(x(value), r.bottom())), color);

	p.setPen(QPen(neutral(this), 1, Qt::DashLine));
	for (double edge : {target - AIRMETER_TOLERANCE_LU, target + AIRMETER_TOLERANCE_LU})
		p.drawLine(QPointF(x(edge), 0), QPointF(x(edge), height()));
}

AirmeterWindow::AirmeterWindow(MeterModel *m) : QWidget(nullptr, Qt::Window), model(m)
{
	setWindowTitle(T("Window.Title"));

	header = new QLabel;
	scaleFont(header, 1.2, true);
	auto *reset = new QPushButton(T("Reset"));
	connect(reset, &QPushButton::clicked, model, &MeterModel::reset);

	auto *top = new QHBoxLayout;
	top->addWidget(header);
	top->addStretch();
	top->addWidget(reset);

	auto *root = new QVBoxLayout(this);
	root->addLayout(top);

	build();

	config_t *c = obs_frontend_get_user_config();
	const char *geometry = c ? config_get_string(c, section, "WindowGeometry") : nullptr;
	if (!geometry || !restoreGeometry(QByteArray::fromBase64(geometry)))
		resize(900, 420);

	connect(model, &MeterModel::updated, this, [this] {
		if (isVisible())
			refresh();
	});
	connect(model, &MeterModel::settingsChanged, this, [this] {
		build();
		refresh();
	});
}

void AirmeterWindow::closeEvent(QCloseEvent *event)
{
	config_t *c = obs_frontend_get_user_config();
	if (c) {
		config_set_string(c, section, "WindowGeometry", saveGeometry().toBase64().constData());
		config_save_safe(c, "tmp", nullptr);
	}
	event->accept();
}

void AirmeterWindow::build()
{
	if (built_layout == model->settings.layout)
		return;
	built_layout = model->settings.layout;

	delete content;
	content = new QWidget;
	tile_s = new ValueTile(T("Value.ShortTerm"), T("Unit.LUFS"));
	tile_m = new ValueTile(T("Value.Momentary"), T("Unit.LUFS"));
	tile_i = new ValueTile(T("Value.Integrated"), T("Unit.LUFS"));
	tile_lra = new ValueTile(T("Value.LRA"), T("Unit.LU"));
	tile_tp = new ValueTile(T("Value.TruePeakMax"), T("Unit.dBTP"));
	graph_sm = graph_s = graph_i = graph_tp = nullptr;

	if (built_layout == 0) {
		auto *tiles = new QHBoxLayout;
		for (ValueTile *t : {tile_s, tile_m, tile_i, tile_lra, tile_tp})
			tiles->addWidget(t);
		graph_sm = new HistoryGraph(T("Graph.ShortTermMomentary"), -40.0, 0.0);

		auto *box = new QVBoxLayout(content);
		box->setContentsMargins(0, 0, 0, 0);
		box->addLayout(tiles);
		box->addWidget(graph_sm, 1);
	} else {
		graph_s = new HistoryGraph(T("Graph.ShortTerm"), -40.0, 0.0);
		graph_i = new HistoryGraph(T("Graph.Integrated"), -40.0, 0.0);
		graph_tp = new HistoryGraph(T("Graph.TruePeak"), -30.0, 3.0);

		auto *graphs = new QVBoxLayout;
		for (HistoryGraph *g : {graph_s, graph_i, graph_tp})
			graphs->addWidget(g, 1);
		auto *tiles = new QVBoxLayout;
		for (ValueTile *t : {tile_s, tile_m, tile_i, tile_lra, tile_tp})
			tiles->addWidget(t);
		tiles->addStretch();

		auto *box = new QHBoxLayout(content);
		box->setContentsMargins(0, 0, 0, 0);
		box->addLayout(graphs, 1);
		box->addLayout(tiles);
	}

	static_cast<QVBoxLayout *>(layout())->addWidget(content, 1);
}

void AirmeterWindow::refresh()
{
	const AirmeterSettings &s = model->settings;
	const loudness_values &v = model->now;
	const History &h = model->history;
	double target = s.target();

	header->setText(QStringLiteral("%1 · %2").arg(trackText(s.track), s.presetLabel()));
	tile_s->setValue(v.s, statusColor(airmeter_status(v.s, target), this));
	tile_m->setValue(v.m, neutral(this));
	tile_i->setValue(v.i, statusColor(airmeter_status(v.i, target), this));
	tile_lra->setValue(v.lra, neutral(this));
	tile_tp->setValue(v.tp_max, peakColor(v.tp_max, s.ceiling, this));

	double low = target - AIRMETER_TOLERANCE_LU;
	double high = target + AIRMETER_TOLERANCE_LU;
	if (graph_sm) {
		graph_sm->setBand(low, high);
		graph_sm->setSeries(
			{{h.s.values(), h.s.capacity, neutral(this), 2.5}, {h.m.values(), h.m.capacity, blue, 1.0}});
	}
	if (graph_s) {
		graph_s->setBand(low, high);
		graph_s->setSeries({{h.s.values(), h.s.capacity, neutral(this), 2.0}});
		graph_i->setBand(low, high);
		graph_i->setSeries({{h.i, 0, neutral(this), 2.0}});
		graph_tp->setCeiling(s.ceiling);
		graph_tp->setSeries({{h.tp.values(), h.tp.capacity, amber, 1.0}});
	}
}

AirmeterDock::AirmeterDock(QWidget *parent) : QWidget(parent), model(new MeterModel(this))
{
	track = new QLabel;
	auto *gear = new QToolButton;
	gear->setText(QStringLiteral("⚙"));
	gear->setToolTip(T("Settings"));
	gear->setAccessibleName(T("Settings"));
	auto *expand = new QToolButton;
	expand->setText(QStringLiteral("⤢"));
	expand->setToolTip(T("OpenWindow"));
	expand->setAccessibleName(T("OpenWindow"));

	auto *top = new QHBoxLayout;
	top->addWidget(track);
	top->addStretch();
	top->addWidget(gear);
	top->addWidget(expand);

	s_value = new QLabel;
	scaleFont(s_value, 2.2, true);
	s_value->setMinimumWidth(s_value->fontMetrics().horizontalAdvance(QStringLiteral("-00.0")));
	status = new QLabel;
	scaleFont(status, 1.1, true);

	auto *main = new QHBoxLayout;
	main->addWidget(new QLabel(QStringLiteral("S")));
	main->addWidget(s_value);
	main->addWidget(new QLabel(T("Unit.LUFS")));
	main->addStretch();
	main->addWidget(status);

	bar = new LoudnessBar;
	m_value = new QLabel;
	i_value = new QLabel;
	tp_value = new QLabel;

	auto *bottom = new QHBoxLayout;
	bottom->addWidget(m_value);
	bottom->addStretch();
	bottom->addWidget(i_value);
	bottom->addStretch();
	bottom->addWidget(tp_value);

	auto *root = new QVBoxLayout(this);
	root->addLayout(top);
	root->addLayout(main);
	root->addWidget(bar);
	root->addLayout(bottom);
	root->addStretch();

	connect(gear, &QToolButton::clicked, this, [this] {
		SettingsDialog dialog(model, this);
		dialog.exec();
	});
	connect(expand, &QToolButton::clicked, this, &AirmeterDock::openWindow);
	connect(model, &MeterModel::updated, this, &AirmeterDock::refresh);
	connect(model, &MeterModel::settingsChanged, this, &AirmeterDock::refresh);
	refresh();
}

void AirmeterDock::refresh()
{
	const AirmeterSettings &s = model->settings;
	const loudness_values &v = model->now;
	double target = s.target();
	enum airmeter_status st = airmeter_status(v.s, target);
	QColor color = statusColor(st, this);

	track->setText(trackText(s.track));
	s_value->setText(fmt(v.s));
	setColor(s_value, color);
	status->setText(QStringLiteral("● %1").arg(statusText(st)));
	setColor(status, color);
	bar->setValue(v.s, target, color);
	m_value->setText(QStringLiteral("M %1").arg(fmt(v.m)));
	i_value->setText(QStringLiteral("I %1").arg(fmt(v.i)));
	tp_value->setText(QStringLiteral("TP %1").arg(fmt(v.tp_max)));
	setColor(tp_value, peakColor(v.tp_max, s.ceiling, this));
}

void AirmeterDock::openWindow()
{
	if (!window)
		window = new AirmeterWindow(model);
	window->show();
	window->raise();
	window->activateWindow();
}

SettingsDialog::SettingsDialog(MeterModel *m, QWidget *parent) : QDialog(parent), model(m)
{
	setWindowTitle(T("Settings"));
	const AirmeterSettings &s = model->settings;

	track = new QComboBox;
	for (int i = 0; i < 6; i++)
		track->addItem(trackText(i));
	track->setCurrentIndex(s.track);

	preset = new QComboBox;
	for (size_t i = 0; i < airmeter_preset_count; i++) {
		const airmeter_preset &p = airmeter_presets[i];
		QString name = T(p.text_key);
		if (!std::isnan(p.target))
			name = QStringLiteral("%1 (%2 %3)").arg(name, fmt(p.target), T("Unit.LUFS"));
		preset->addItem(name, QString::fromUtf8(p.id));
	}
	preset->setCurrentIndex(preset->findData(QString::fromStdString(s.preset)));

	custom_target = new QDoubleSpinBox;
	custom_target->setRange(-30.0, -5.0);
	custom_target->setDecimals(1);
	custom_target->setSingleStep(0.5);
	custom_target->setSuffix(QStringLiteral(" %1").arg(T("Unit.LUFS")));
	custom_target->setValue(s.custom_target);

	ceiling = new QDoubleSpinBox;
	ceiling->setRange(-3.0, 0.0);
	ceiling->setDecimals(1);
	ceiling->setSingleStep(0.1);
	ceiling->setSuffix(QStringLiteral(" %1").arg(T("Unit.dBTP")));
	ceiling->setValue(s.ceiling);

	layout = new QComboBox;
	layout->addItem(T("Layout.A"));
	layout->addItem(T("Layout.B"));
	layout->setCurrentIndex(s.layout);

	auto enableCustom = [this] {
		custom_target->setEnabled(
			std::isnan(airmeter_find_preset(qPrintable(preset->currentData().toString()))->target));
	};
	enableCustom();
	connect(preset, &QComboBox::currentIndexChanged, this, enableCustom);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttons, &QDialogButtonBox::accepted, this, [this] {
		save();
		accept();
	});
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto *form = new QFormLayout(this);
	form->addRow(T("Settings.Track"), track);
	form->addRow(T("Settings.Preset"), preset);
	form->addRow(T("Settings.CustomTarget"), custom_target);
	form->addRow(T("Settings.Ceiling"), ceiling);
	form->addRow(T("Settings.Layout"), layout);
	form->addRow(buttons);
}

void SettingsDialog::save()
{
	AirmeterSettings s = model->settings;
	s.track = track->currentIndex();
	s.preset = preset->currentData().toString().toStdString();
	s.custom_target = custom_target->value();
	s.ceiling = ceiling->value();
	s.layout = layout->currentIndex();
	model->apply(s);
}

static QPointer<AirmeterDock> dock;

static void on_event(enum obs_frontend_event event, void *)
{
	if (!dock)
		return;
	if (event == OBS_FRONTEND_EVENT_STREAMING_STARTING)
		dock->model->reset();
	else if (event == OBS_FRONTEND_EVENT_EXIT && dock->window) {
		dock->window->close();
		delete dock->window;
	}
}

extern "C" bool airmeter_ui_load(void)
{
	dock = new AirmeterDock(static_cast<QWidget *>(obs_frontend_get_main_window()));
	if (!meter_start((size_t)dock->model->settings.track) ||
	    !obs_frontend_add_dock_by_id("obs-airmeter", obs_module_text("Dock.Title"), dock)) {
		meter_stop();
		delete dock;
		return false;
	}
	obs_frontend_add_event_callback(on_event, nullptr);
	return true;
}

extern "C" void airmeter_ui_unload(void)
{
	obs_frontend_remove_event_callback(on_event, nullptr);
}

/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "GeoFenceController.h"
#include "Vehicle.h"
#include "FirmwarePlugin.h"
#include "MAVLinkProtocol.h"
#include "QGCApplication.h"
#include "ParameterManager.h"
#include "JsonHelper.h"
#include "QGCQGeoCoordinate.h"
#include "AppSettings.h"

#ifndef __mobile__
#include "MainWindow.h"
#include "QGCQFileDialog.h"
#endif

#include <QJsonDocument>
#include <QJsonArray>

QGC_LOGGING_CATEGORY(GeoFenceControllerLog, "GeoFenceControllerLog")

const char* GeoFenceController::_jsonFileTypeValue =    "GeoFence";
const char* GeoFenceController::_jsonBreachReturnKey =  "breachReturn";

GeoFenceController::GeoFenceController(PlanMasterController* masterController, QObject* parent)
    : PlanElementController(masterController, parent)
    , _geoFenceManager(_managerVehicle->geoFenceManager())
    , _dirty(false)
    , _mapPolygon(this)
{
    connect(_mapPolygon.qmlPathModel(), &QmlObjectListModel::countChanged, this, &GeoFenceController::_updateContainsItems);
    connect(_mapPolygon.qmlPathModel(), &QmlObjectListModel::dirtyChanged, this, &GeoFenceController::_polygonDirtyChanged);

    managerVehicleChanged(_managerVehicle);
}

GeoFenceController::~GeoFenceController()
{

}

void GeoFenceController::start(bool editMode)
{
    qCDebug(GeoFenceControllerLog) << "start editMode" << editMode;

    PlanElementController::start(editMode);
    _init();
}

void GeoFenceController::_init(void)
{

}

void GeoFenceController::setBreachReturnPoint(const QGeoCoordinate& breachReturnPoint)
{
    if (_breachReturnPoint != breachReturnPoint) {
        _breachReturnPoint = breachReturnPoint;
        setDirty(true);
        emit breachReturnPointChanged(breachReturnPoint);
    }
}

void GeoFenceController::_signalAll(void)
{
    emit breachReturnSupportedChanged(breachReturnSupported());
    emit breachReturnPointChanged(breachReturnPoint());
    emit circleEnabledChanged(circleEnabled());
    emit circleRadiusFactChanged(circleRadiusFact());
    emit polygonEnabledChanged(polygonEnabled());
    emit polygonSupportedChanged(polygonSupported());
    emit dirtyChanged(dirty());
}

void GeoFenceController::managerVehicleChanged(Vehicle* managerVehicle)
{
    if (_managerVehicle) {
        _geoFenceManager->disconnect(this);
        _managerVehicle = NULL;
        _geoFenceManager = NULL;
    }

    _managerVehicle = managerVehicle;
    if (!_managerVehicle) {
        qWarning() << "GeoFenceController::managerVehicleChanged managerVehicle=NULL";
        return;
    }

    _geoFenceManager = _managerVehicle->geoFenceManager();
    connect(_geoFenceManager, &GeoFenceManager::breachReturnSupportedChanged,   this, &GeoFenceController::breachReturnSupportedChanged);
    connect(_geoFenceManager, &GeoFenceManager::circleEnabledChanged,           this, &GeoFenceController::circleEnabledChanged);
    connect(_geoFenceManager, &GeoFenceManager::circleRadiusFactChanged,        this, &GeoFenceController::circleRadiusFactChanged);
    connect(_geoFenceManager, &GeoFenceManager::polygonEnabledChanged,          this, &GeoFenceController::polygonEnabledChanged);
    connect(_geoFenceManager, &GeoFenceManager::polygonSupportedChanged,        this, &GeoFenceController::polygonSupportedChanged);
    connect(_geoFenceManager, &GeoFenceManager::loadComplete,                   this, &GeoFenceController::_loadComplete);
    connect(_geoFenceManager, &GeoFenceManager::inProgressChanged,              this, &GeoFenceController::syncInProgressChanged);

    _signalAll();
}

bool GeoFenceController::load(const QJsonObject& json, QString& errorString)
{
    QString errorStr;
    QString errorMessage = tr("GeoFence: %1");

    if (json.contains(_jsonBreachReturnKey) &&
            !JsonHelper::loadGeoCoordinate(json[_jsonBreachReturnKey], false /* altitudeRequired */, _breachReturnPoint, errorStr)) {
        errorString = errorMessage.arg(errorStr);
        return false;
    }

    if (!_mapPolygon.loadFromJson(json, true, errorStr)) {
        errorString = errorMessage.arg(errorStr);
        return false;
    }
    _mapPolygon.setDirty(false);
    setDirty(false);

    _signalAll();

    return true;
}

void  GeoFenceController::save(QJsonObject& json)
{
    json[JsonHelper::jsonVersionKey] = 1;

    if (_breachReturnPoint.isValid()) {
        QJsonValue jsonBreachReturn;
        JsonHelper::saveGeoCoordinate(_breachReturnPoint, false /* writeAltitude */, jsonBreachReturn);
        json[_jsonBreachReturnKey] = jsonBreachReturn;
    }

    _mapPolygon.saveToJson(json);
}

void GeoFenceController::removeAll(void)
{
    setBreachReturnPoint(QGeoCoordinate());
    _mapPolygon.clear();
}

void GeoFenceController::loadFromVehicle(void)
{
    if (!syncInProgress()) {
        _geoFenceManager->loadFromVehicle();
    } else {
        qCWarning(GeoFenceControllerLog) << "GeoFenceController::loadFromVehicle call while syncInProgress";
    }
}

void GeoFenceController::sendToVehicle(void)
{
    _geoFenceManager->sendToVehicle(_breachReturnPoint, _mapPolygon.pathModel());
    _mapPolygon.setDirty(false);
    setDirty(false);
}

bool GeoFenceController::syncInProgress(void) const
{
    return _geoFenceManager->inProgress();
}

bool GeoFenceController::dirty(void) const
{
    return _dirty;
}


void GeoFenceController::setDirty(bool dirty)
{
    if (dirty != _dirty) {
        _dirty = dirty;
        if (!dirty) {
            _mapPolygon.setDirty(dirty);
        }
        emit dirtyChanged(dirty);
    }
}

void GeoFenceController::_polygonDirtyChanged(bool dirty)
{
    if (dirty) {
        setDirty(true);
    }
}

bool GeoFenceController::breachReturnSupported(void) const
{
    return _geoFenceManager->breachReturnSupported();
}

bool GeoFenceController::circleEnabled(void) const
{
    return _geoFenceManager->circleEnabled();
}

Fact* GeoFenceController::circleRadiusFact(void) const
{
    return _geoFenceManager->circleRadiusFact();
}

bool GeoFenceController::polygonSupported(void) const
{
    return _geoFenceManager->polygonSupported();
}

bool GeoFenceController::polygonEnabled(void) const
{
    return _geoFenceManager->polygonEnabled();
}

QVariantList GeoFenceController::params(void) const
{
    return _geoFenceManager->params();
}

QStringList GeoFenceController::paramLabels(void) const
{
    return _geoFenceManager->paramLabels();
}

void GeoFenceController::_setDirty(void)
{
    setDirty(true);
}

void GeoFenceController::_setPolygonFromManager(const QList<QGeoCoordinate>& polygon)
{
    _mapPolygon.clear();
    for (int i=0; i<polygon.count(); i++) {
        _mapPolygon.appendVertex(polygon[i]);
    }
    _mapPolygon.setDirty(false);
}

void GeoFenceController::_setReturnPointFromManager(QGeoCoordinate breachReturnPoint)
{
    _breachReturnPoint = breachReturnPoint;
    emit breachReturnPointChanged(_breachReturnPoint);
}

void GeoFenceController::_loadComplete(const QGeoCoordinate& breachReturn, const QList<QGeoCoordinate>& polygon)
{
    _setReturnPointFromManager(breachReturn);
    _setPolygonFromManager(polygon);
    setDirty(false);
    emit loadComplete();
}

bool GeoFenceController::containsItems(void) const
{
    return _mapPolygon.count() > 2;
}

void GeoFenceController::_updateContainsItems(void)
{
    emit containsItemsChanged(containsItems());
}

void GeoFenceController::removeAllFromVehicle(void)
{
    _geoFenceManager->removeAll();
}

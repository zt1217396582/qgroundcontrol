/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef PlanElementController_H
#define PlanElementController_H

#include <QObject>

#include "Vehicle.h"
#include "MultiVehicleManager.h"

class PlanMasterController;

/// This is the abstract base clas for Plan Element controllers.
/// Examples of plan elements are: missions (MissionController), geofence (GeoFenceController)
class PlanElementController : public QObject
{
    Q_OBJECT
    
public:
    PlanElementController(PlanMasterController* masterController, QObject* parent = NULL);
    ~PlanElementController();
    
    Q_PROPERTY(bool containsItems   READ containsItems                  NOTIFY containsItemsChanged)    ///< true: Elemement is non-empty
    Q_PROPERTY(bool syncInProgress  READ syncInProgress                 NOTIFY syncInProgressChanged)   ///< true: information is currently being saved/sent, false: no active save/send in progress
    Q_PROPERTY(bool dirty           READ dirty          WRITE setDirty  NOTIFY dirtyChanged)            ///< true: unsaved/sent changes are present, false: no changes since last save/send

    /// Should be called immediately upon Component.onCompleted.
    ///     @param editMode true: controller being used in Plan view, false: controller being used in Fly view
    virtual void start(bool editMode);

    virtual void save(QJsonObject& json) = 0;
    virtual bool load(const QJsonObject& json, QString& errorString) = 0;
    virtual void loadFromVehicle(void) = 0;
    virtual void sendToVehicle(void) = 0;
    virtual void removeAll(void) = 0;                       ///< Removes all from controller only, synce required to remove from vehicle
    virtual void removeAllFromVehicle(void) = 0;            ///< Removes all from vehicle and controller

    virtual bool    containsItems  (void) const = 0;
    virtual bool    syncInProgress (void) const = 0;
    virtual bool    dirty          (void) const = 0;
    virtual void    setDirty       (bool dirty) = 0;

    /// Called when a new manager vehicle has been set. Derived classes should override to implement custom behavior.
    virtual void managerVehicleChanged(Vehicle* managerVehicle) = 0;

signals:
    void containsItemsChanged   (bool containsItems);
    void syncInProgressChanged  (bool syncInProgress);
    void dirtyChanged           (bool dirty);
    void vehicleChanged         (Vehicle* vehicle);

protected:
    PlanMasterController*   _masterController;
    Vehicle*                _controllerVehicle;
    Vehicle*                _managerVehicle;
    bool                    _editMode;
};

#endif

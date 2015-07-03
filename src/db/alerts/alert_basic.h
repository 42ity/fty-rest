/*
Copyright (C) 2014-2015 Eaton
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*! \file alert_basic.h
    \brief Pure DB API for CRUD operations on alerts

    \author Alena Chernikava <AlenaChernikava@eaton.com>
*/

#ifndef SRC_DB_ALERTS_ALERT_BASIC
#define SRC_DB_ALERTS_ALERT_BASIC

#include <tntdb/connect.h>
#include "dbtypes.h"
#include "dbhelpers.h"

namespace persist {

//+
/*
 * \brief Inserts a new record into the table [t_bios_alert]
 *        about new alert discovered in the system.
 *
 *  An end date of the alert can't be specified during the insert statement,
 *  because alert just started.
 *
 * \param conn          - a connection to the database
 * \param rule_name     - a rule name that produced this alert
 * \param priority      - a priority of the alert
 * \param alert_state   - an alert state
 * \param description   - a description of the alert (can be NULL)
 * \param notification  - a bit-vector about notifications already sent 
 *                        about this alert
 * \param date_from     - a date+time when system evaluated that 
 *                        this alert had started
 * 
 * \return in case of success: status = 1,
 *                             rowid is set,
 *                             affected_rows is set
 *         in case of fail:    status = 0,
 *                             errtype is set,
 *                             errsubtype is set,
 *                             msg is set
 */
db_reply_t
    insert_into_alert 
        (tntdb::Connection  &conn,
         const char         *rule_name,
         a_elmnt_pr_t        priority,
         m_alrt_state_t      alert_state,
         const char         *description,
         m_alrt_ntfctn_t     notification,
         int64_t             date_from);

//+
/*
 * \brief Updates the record in the table [t_bios_alert] by alert_id.
 *        Sets a new notification bit-vector.
 *
 *  Sets a new notification bit-vector. It is able only to add 
 *  a new notifications sended. (Because we cannot "unsend" a notification).
 *
 * \param conn          - a connection to the database
 * \param notification  - a bit-vector about new notifications sent 
 *                        about this alert
 * \param alert_id      - an id of the alert in the database
 * 
 * \return in case of success: status = 1,
 *                             rowid is set(is 0, if alert_id doesn't exist),
 *                             affected_rows is set
 *         in case of fail:    status = 0,
 *                             errtype is set,
 *                             errsubtype is set,
 *                             msg is set
 */
db_reply_t
    update_alert_notification_byId 
        (tntdb::Connection  &conn,
         m_alrt_ntfctn_t     notification,
         m_alrt_id_t         alert_id);

//+
/*
 * \brief Updates the record in the table [t_bios_alert] by alert_id.
 *        Sets an end date of the alert.
 *
 * An end date would be updated only if it was not specified yet
 * (value of the date_till column was NULL).
 *
 * \param conn          - a connection to the database
 * \param date_till     - a date+time when system evaluated that 
 *                        this alert had ended
 * \param alert_id      - an id of the alert in the database
 * 
 * \return in case of success: status = 1,
 *                             rowid is set(is 0, if alert_id doesn't exist),
 *                             affected_rows is set
 *         in case of fail:    status = 0,
 *                             errtype is set,
 *                             errsubtype is set,
 *                             msg is set
 */
db_reply_t
    update_alert_tilldate 
        (tntdb::Connection  &conn,
         int64_t             date_till,
         m_alrt_id_t         alert_id);

//+
/*
 * \brief Updates the record in the table [t_bios_alert] by rule name.
 *        Sets an end date of the alert.
 *
 * An end date would be updated only if it was not specified yet
 * (value of the date_till column was NULL).
 *
 * \param conn          - a connection to the database
 * \param date_till     - a date+time when system evaluated that 
 *                        this alert had ended
 * \param rule name     - an rule name the alert was generated by
 * 
 * \return in case of success: status = 1,
 *                             rowid is set(is 0, if alert_id doesn't exist),
 *                             affected_rows is set
 *         in case of fail:    status = 0,
 *                             errtype is set,
 *                             errsubtype is set,
 *                             msg is set
 */
db_reply_t
    update_alert_tilldate_by_rulename
        (tntdb::Connection  &conn,
         int64_t             date_till,
         const char         *rule_name);

//+
/*
 * \brief Deletes a record from the table [t_bios_alert] by alert_id.
 *
 * Only if there is no child records in t_bios_alert_device.
 *
 * \param conn          - a connection to the database
 * \param alert_id      - an id of the row to be deleted 
 * 
 * \return in case of success: status = 1,
 *                             affected_rows is set
 *         in case of fail:    status = 0,
 *                             errtype is set,
 *                             errsubtype is set,
 *                             msg is set
 */
db_reply_t
    delete_from_alert
        (tntdb::Connection &conn,
         m_alrt_id_t        alert_id);

//+
/*
 * \brief Inserts new records into the table [t_bios_alert_device]
 *        about what devices where used to evaluate an alert.
 *
 *  Trying to insert as mush devices as possible, in case of error returns
 *  the information about first error happend.
 *
 * \param conn          - a connection to the database
 * \param alert_id      - an id of the alert
 * \param device_names  - a list of devices used to evaluate the alert
 * 
 * \return in case of success: status = 1,
 *                             affected_rows is set
 *         in case of fail:    status = 0,
 *                             affected_rows is a number of 
 *                                      actually inserted rows,
 *                             errtype is set as first error occures,
 *                             errsubtype is set as first error occures,
 *                             msg is set as first erorr occures.
 */
db_reply_t
    insert_into_alert_devices 
        (tntdb::Connection        &conn,
         m_alrt_id_t               alert_id,
         std::vector <std::string> device_names);

//+
/*
 * \brief Inserts a new record into the table [t_bios_alert_device].
 *
 * \param conn          - a connection to the database
 * \param alert_id      - a id of the alert
 * \param device_names  - a name of the device used to evaluate the alert 
 * 
 * \return in case of success: status = 1,
 *                             affected_rows is set
 *                             rowid is set
 *         in case of fail:    status = 0,
 *                             errtype is set,
 *                             errsubtype is set,
 *                             msg is set.
 */
db_reply_t
    insert_into_alert_device
        (tntdb::Connection &conn,
         m_alrt_id_t        alert_id,
         const char        *device_name);

//+
/*
 * \brief Deletes a record from the table [t_bios_alert_device] by id.
 *
 * \param conn          - a connection to the database
 * \param id            - an id of the row to be deleted 
 * 
 * \return in case of success: status = 1,
 *                             affected_rows is set
 *         in case of fail:    status = 0,
 *                             errtype is set,
 *                             errsubtype is set,
 *                             msg is set
 */
db_reply_t
    delete_from_alert_device
        (tntdb::Connection &conn,
         m_alrtdvc_id_t    id);

//+
/*
 * \brief Deletes records from the table [t_bios_alert_device]
 *          that belongs to one alert specified by its alert_id.
 *
 * \param conn          - a connection to the database
 * \param alert_id      - an id of the alert 
 * 
 * \return in case of success: status = 1,
 *                             affected_rows is set
 *         in case of fail:    status = 0,
 *                             errtype is set,
 *                             errsubtype is set,
 *                             msg is set
 */
db_reply_t
    delete_from_alert_device_byalert
        (tntdb::Connection &conn,
         m_alrt_id_t        alert_id);

//+
/*
 * \brief Inserts a new record into the table [t_bios_alert_device].
 *
 * \param conn          - a connection to the database
 * \param rule_name     - a rule name that produced this alert
 * \param priority      - a priority of the alert
 * \param alert_state   - an alert state
 * \param description   - a description of the alert (can be NULL)
 * \param notification  - a bit-vector about notifications already sent 
 *                        about this alert
 * \param date_from     - a date+time when system evaluated that 
 *                        this alert had started
 * \param device_names  - a name of the device used to evaluate the alert 
 *
 * \return in case of success: status = 1,
 *                             affected_rows is set
 *                             rowid is set
 *         in case of fail:    status = 0,
 *                             errtype is set,
 *                             errsubtype is set,
 *                             msg is set.
 */
db_reply_t
    insert_new_alert 
        (tntdb::Connection  &conn,
         const char         *rule_name,
         a_elmnt_pr_t        priority,
         m_alrt_state_t      alert_state,
         const char         *description,
         m_alrt_ntfctn_t     notification,
         int64_t             date_from,
         std::vector<std::string> device_names);

//+
/*
 * \brief Selects all opened alerts.
 *
 * Regardless datacenter!!!
 * TODO: redo a selection of alerts for multiple
 * datacenters
 *
 * \param conn          - a connection to the database
 *
 * \return in case of success: status = 1,
 *                             item is set.
 *         in case of fail:    status = 0,
 *                             errtype is set,
 *                             errsubtype is set,
 *                             msg is set.
 */
db_reply <std::vector<db_alert_t>>
    select_alert_all_opened
        (tntdb::Connection  &conn);

//+
/*
 * \brief Selects all closed alerts.
 *
 * Regardless datacenter!!!
 * TODO: redo a selection of alerts for multiple
 * datacenters
 *
 * \param conn          - a connection to the database
 *
 * \return in case of success: status = 1,
 *                             item is set.
 *         in case of fail:    status = 0,
 *                             errtype is set,
 *                             errsubtype is set,
 *                             msg is set.
 */
db_reply <std::vector<db_alert_t>>
    select_alert_all_closed
        (tntdb::Connection  &conn);

//+
/*
 * \brief Selects all devices that were used in evaluation of the alert.
 *
 * \param conn          - a connection to the database
 * \param alert_id      - an id of the alert 
 *
 * \return in case of success: status = 1,
 *                             item is set.
 *         in case of fail:    status = 0,
 *                             errtype is set,
 *                             errsubtype is set,
 *                             msg is set.
 */
db_reply <std::vector<m_dvc_id_t>>
    select_alert_devices
        (tntdb::Connection &conn,
         m_alrt_id_t        alert_id);


db_reply <db_alert_t>
    select_alert_last_byRuleName
        (tntdb::Connection &conn,
         const char *rule_name);

db_reply <db_alert_t>
    select_alert_byRuleNameDateFrom
        (tntdb::Connection &conn,
         const char *rule_name,
         int64_t     date_from);

db_reply_t
    update_alert_notification_byRuleName 
        (tntdb::Connection  &conn,
         m_alrt_ntfctn_t     notification,
         const char *rule_name);


 
} //namespace persist

#endif //SRC_DB_ALERTS_ALERT_BASIC

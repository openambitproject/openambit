#include "sportmode.h"
#include "libambit.h"

const QString CustomMode::ACTIVITY_ID = "ActivityID";
const QString CustomMode::ALTI_BARO_MODE = "AltiBaroMode";
const QString CustomMode::AUTOLAP_DISTANCE = "AutolapDistance";
const QString CustomMode::GPS_INTERVAL = "GPSInterval";
const QString CustomMode::HR_LIMIT_HIGH = "HRLimitHigh";
const QString CustomMode::HR_LIMIT_LOW = "HRLimitLow";
const QString CustomMode::INTERVAL_1_DISTANCE = "Interval1Distance";
const QString CustomMode::INTERVAL_2_DISTANCE = "Interval2Distance";
const QString CustomMode::INTERVAL_1_TIME = "Interval1Time";
const QString CustomMode::INTERVAL_2_TIME = "Interval2Time";
const QString CustomMode::INTERVAL_REPETITIONS = "IntervalRepetitions";
const QString CustomMode::ACTIVITY_NAME = "Name";
const QString CustomMode::RECORDING_INTERVAL = "RecordingInterval";
const QString CustomMode::USE_ACCELEROMETER = "UseAccelerometer";
const QString CustomMode::USE_AUTOLAP = "UseAutolap";
const QString CustomMode::USE_BIKE_POD = "UseBikePOD";
const QString CustomMode::USE_CADENS_POD = "UseCadencePOD";
const QString CustomMode::USE_FOOT_POD = "UseFootPOD";
const QString CustomMode::USE_POWER_POD = "UsePowerPOD";
const QString CustomMode::USE_HR_BELT = "UseHRBelt";
const QString CustomMode::USE_HR_LIMITS = "UseHRLimits";
const QString CustomMode::USE_INTERVALS = "UseIntervals";
const QString CustomMode::SPORT_MODE_ID = "CustomModeID";
const QString CustomMode::AUTO_SCROLING_SPEED = "AutoScrolling";
const QString CustomMode::AUTO_PAUSE_SPEED = "AutoPauseSpeed";
const QString CustomMode::BACKLIGHT_MODE = "BacklightMode";
const QString CustomMode::DISPLAY_IS_NEGATIVE = "DisplayIsNegative";
const QString CustomMode::SHOW_NAVIGATION_SELECTION = "ShowNavigationSelection";
const QString CustomMode::DISPLAY = "Displays";
const QString CustomMode::DISPLAYED_RULE_IDS = "DisplayedRuleIDs";
const QString CustomMode::LOGGED_RULE_IDS = "LoggedRuleIDs";


CustomMode::CustomMode(QVariantMap &customModeMap, QObject *parent) :
    QObject(parent)
{
    activityId = customModeMap[ACTIVITY_ID].toUInt();
    altiBaroMode = customModeMap[ALTI_BARO_MODE].toUInt();
    autolapDistance = customModeMap[AUTOLAP_DISTANCE].toUInt();
    gpsInterval = customModeMap[GPS_INTERVAL].toUInt();
    hrLimitHigh = customModeMap[HR_LIMIT_HIGH].toUInt();
    hrLimitLow = customModeMap[HR_LIMIT_LOW].toUInt();
    interval1distance = customModeMap[INTERVAL_1_DISTANCE].toUInt();
    interval2distance = customModeMap[INTERVAL_2_DISTANCE].toUInt();
    interval1time = customModeMap[INTERVAL_1_TIME].toUInt();
    interval2time = customModeMap[INTERVAL_2_TIME].toUInt();
    intervalRepetitions = customModeMap[INTERVAL_REPETITIONS].toUInt();
    activityName = customModeMap[ACTIVITY_NAME].toString();
    recordingInterval = customModeMap[RECORDING_INTERVAL].toUInt();
    useAccelerometer = customModeMap[USE_ACCELEROMETER].toBool();
    useAutolap = customModeMap[USE_AUTOLAP].toBool();
    useBikePod = customModeMap[USE_BIKE_POD].toBool();
    useCadencePod = customModeMap[USE_CADENS_POD].toBool();
    useFootPod = customModeMap[USE_FOOT_POD].toBool();
    usePowerPod = customModeMap[USE_POWER_POD].toBool();
    useHrBelt = customModeMap[USE_HR_BELT].toBool();
    useHrLimits = customModeMap[USE_HR_LIMITS].toBool();
    useIntervals = customModeMap[USE_INTERVALS].toBool();
    sportmodeId = customModeMap[SPORT_MODE_ID].toUInt();
    if (customModeMap[AUTO_SCROLING_SPEED].toString() == QString::null) {
        autoScrolingSpeed = 0;
    }
    else {
        autoScrolingSpeed = customModeMap[AUTO_SCROLING_SPEED].toUInt();
    }
    autoPauseSpeed = customModeMap[AUTO_PAUSE_SPEED].toFloat();
    if (customModeMap[BACKLIGHT_MODE].toString() == QString::null) {
        backlightMode = 0xff;
    }
    else {
        backlightMode = customModeMap[BACKLIGHT_MODE].toUInt();
    }
    if (customModeMap[DISPLAY_IS_NEGATIVE].toString() == QString::null) {
        displayIsNegative = 0xff;
    }
    else {
        displayIsNegative = customModeMap[DISPLAY_IS_NEGATIVE].toUInt();
    }
    showNavigationSelection = customModeMap[SHOW_NAVIGATION_SELECTION].toUInt();

    foreach (QVariant displayVar, customModeMap[DISPLAY].toList()) {
        QVariantMap displayMap = displayVar.toMap();

        CustomModeDisplay display(displayMap, this->parent());
        displays.append(display);
    }

    foreach (QVariant ruleIdsVar, customModeMap[DISPLAYED_RULE_IDS].toList()) {
        int ruleId = ruleIdsVar.toInt();
        appRuleIds.append(ruleId);
    }

    foreach (QVariant loggedRuleIdsVar, customModeMap[LOGGED_RULE_IDS].toList()) {
        int ruleId = loggedRuleIdsVar.toInt();
        loggedAppRuleIds.append(ruleId);
    }
}

CustomMode::CustomMode(const CustomMode &other) :
    QObject(other.parent()),
    activityId(other.activityId),
    altiBaroMode(other.altiBaroMode),
    autolapDistance(other.autolapDistance),
    gpsInterval(other.gpsInterval),
    hrLimitHigh(other.hrLimitHigh),
    hrLimitLow(other.hrLimitLow),
    interval1distance(other.interval1distance),
    interval2distance(other.interval2distance),
    interval1time(other.interval1time),
    interval2time(other.interval2time),
    intervalRepetitions(other.intervalRepetitions),
    activityName(other.activityName),
    recordingInterval(other.recordingInterval),
    useAccelerometer(other.useAccelerometer),
    useAutolap(other.useAutolap),
    useBikePod(other.useBikePod),
    useCadencePod(other.useCadencePod),
    useFootPod(other.useFootPod),
    usePowerPod(other.usePowerPod),
    useHrBelt(other.useHrBelt),
    useHrLimits(other.useHrLimits),
    useIntervals(other.useIntervals),
    sportmodeId(other.sportmodeId),
    autoScrolingSpeed(other.autoScrolingSpeed),
    autoPauseSpeed(other.autoPauseSpeed),
    backlightMode(other.backlightMode),
    displayIsNegative(other.displayIsNegative),
    showNavigationSelection(other.showNavigationSelection),
    displays(other.displays),
    appRuleIds(other.appRuleIds),
    loggedAppRuleIds(other.loggedAppRuleIds)
{
}

CustomMode &CustomMode::operator=(const CustomMode &rhs)
{
    activityId = rhs.activityId;
    altiBaroMode = rhs.altiBaroMode;
    autolapDistance = rhs.autolapDistance;
    gpsInterval = rhs.gpsInterval;
    hrLimitHigh = rhs.hrLimitHigh;
    hrLimitLow = rhs.hrLimitLow;
    interval1distance = rhs.interval1distance;
    interval2distance = rhs.interval2distance;
    interval1time = rhs.interval1time;
    interval2time = rhs.interval2time;
    intervalRepetitions = rhs.intervalRepetitions;
    activityName = rhs.activityName;
    recordingInterval = rhs.recordingInterval;
    useAccelerometer = rhs.useAccelerometer;
    useAutolap = rhs.useAutolap;
    useBikePod = rhs.useBikePod;
    useCadencePod = rhs.useCadencePod;
    useFootPod = rhs.useFootPod;
    usePowerPod = rhs.usePowerPod;
    useHrBelt = rhs.useHrBelt;
    useHrLimits = rhs.useHrLimits;
    useIntervals = rhs.useIntervals;
    sportmodeId = rhs.sportmodeId;
    autoScrolingSpeed = rhs.autoScrolingSpeed;
    autoPauseSpeed = rhs.autoPauseSpeed;
    backlightMode = rhs.backlightMode;
    displayIsNegative = rhs.displayIsNegative;
    showNavigationSelection = rhs.showNavigationSelection;
    displays = rhs.displays;
    appRuleIds = rhs.appRuleIds;
    loggedAppRuleIds = rhs.loggedAppRuleIds;

    return *this;
}

void CustomMode::toAmbitCustomModeData(ambit_sport_mode_t *ambitCustomMode, ambit_sport_mode_device_settings_t *ambitSettings)
{
    // Copy settings
    ambit_sport_mode_settings_t *ambitCustomModeSettings = &(ambitCustomMode->settings);
    toAmbitSettings(ambitCustomModeSettings);

    // Copy displays
    if (libambit_malloc_sport_mode_displays(displays.count(), ambitCustomMode)) {
        ambit_sport_mode_display_t *ambitDisplays = ambitCustomMode->display;

        foreach (CustomModeDisplay display, displays) {
            display.toAmbitCustomModeData(ambitDisplays);
            ambitDisplays++;
        }
    }

    if (libambit_malloc_sport_mode_app_ids(appRuleIds.count(), ambitCustomMode)) {
        for(int i = 0; i < appRuleIds.count(); i++) {
            ambitCustomMode->apps_list[i].index = ambitSettings->app_ids_count;
            ambitCustomMode->apps_list[i].logging = (loggedAppRuleIds.size() > i && loggedAppRuleIds.at(i) != 0);

            ambitSettings->app_ids[ambitSettings->app_ids_count] = appRuleIds.at(i);
            ambitSettings->app_ids_count++;
        }
    }
}

void CustomMode::toAmbitSettings(ambit_sport_mode_settings_t *settings)
{
    toAmbitName(settings->activity_name);
    settings->activity_id = (uint16_t)activityId;
    settings->sport_mode_id = (uint16_t)sportmodeId;
    memset(settings->unknown1, 0, sizeof(settings->unknown1));
    settings->hrbelt_and_pods = hrbeltAndPods();
    settings->alti_baro_mode = altiBaroMode;
    settings->gps_interval = gpsInterval;
    settings->recording_interval = recordingInterval;
    settings->autolap = useAutolap ? autolapDistance : 0;
    settings->heartrate_max = hrLimitHigh;
    settings->heartrate_min = hrLimitLow;
    settings->use_heartrate_limits = useHrLimits;
    memset(settings->unknown2, 0, sizeof(settings->unknown2));
    settings->auto_pause = autoPauseSpeed * 100;
    settings->auto_scroll = autoScrolingSpeed;
    settings->use_interval_timer = useIntervals ? 1 : 0;
    settings->interval_repetitions = intervalRepetitions;
    settings->interval_timer_max_unit = interval1time ? 0x0100 : 0;
    memset(settings->unknown3, 0, sizeof(settings->unknown3));
    settings->interval_timer_max = interval1distance ? interval1distance : interval1time;
    memset(settings->unknown4, 0, sizeof(settings->unknown4));
    settings->interval_timer_min_unit = interval2time ? 0x0100 : 0;
    memset(settings->unknown5, 0, sizeof(settings->unknown5));
    settings->interval_timer_min = interval2distance ? interval2distance : interval2time;
    memset(settings->unknown6, 0, sizeof(settings->unknown6));
    settings->backlight_mode = backlightMode;
    settings->display_mode = displayIsNegative;
    settings->quick_navigation = showNavigationSelection;
}

void CustomMode::toAmbitName(char ambitName[NAME_SIZE])
{
    const QByteArray &source = activityName.toLatin1();
    int strLen = activityName.length() < NAME_SIZE ? activityName.length() : NAME_SIZE;
    memset(ambitName, 0x00, NAME_SIZE);
    memcpy(ambitName, source.data(), strLen);
}

u_int16_t CustomMode::hrbeltAndPods()
{
    u_int16_t retVal = 0;

    if (useHrBelt)          { retVal |= 0x0003; }
    if (useAccelerometer)   { retVal |= 0x0004; }
    if (usePowerPod)        { retVal |= 0x0042; }
    if (useCadencePod)      { retVal |= 0x0082; }
    if (useFootPod)         { retVal |= 0x0102; }
    if (useBikePod)         { retVal |= 0x0802; }

    return retVal;
}

uint CustomMode::getCustomModeId() const
{
    return sportmodeId;
}

const QString CustomModeDisplay::REQUIRES_HR_BELT = "RequiresHRBelt";
const QString CustomModeDisplay::ROW_1 = "Row1";
const QString CustomModeDisplay::ROW_2 = "Row2";
const QString CustomModeDisplay::TYPE = "Type";
const QString CustomModeDisplay::VIEWS = "Views";

CustomModeDisplay::CustomModeDisplay(QVariantMap &displayMap, QObject *parent) :
    QObject(parent)
{
    bool ok = false;
    requiresHRBelt = displayMap[REQUIRES_HR_BELT].toBool();
    row1 = displayMap[ROW_1].toInt(&ok);
    if (!ok) {
        row1 = -1;
    }
    row2 = displayMap[ROW_2].toInt(&ok);
    if (!ok) {
        row2 = -1;
    }
    type = displayMap[TYPE].toInt();

    foreach (QVariant viewVar, displayMap[VIEWS].toList()) {
        int view = viewVar.toInt();
        views.append(view);
    }
}

CustomModeDisplay::CustomModeDisplay(const CustomModeDisplay &other) :
    QObject(other.parent()),
    requiresHRBelt(other.requiresHRBelt),
    row1(other.row1),
    row2(other.row2),
    type(other.type),
    views(other.views)
{
}

CustomModeDisplay &CustomModeDisplay::operator=(const CustomModeDisplay &rhs)
{
    requiresHRBelt = rhs.requiresHRBelt;
    row1 = rhs.row1;
    row2 = rhs.row2;
    type = rhs.type;
    views = rhs.views;

    return *this;
}

void CustomModeDisplay::toAmbitCustomModeData(ambit_sport_mode_display_t *ambitDisplay)
{
    ambitDisplay->type = ambitDisplayType();
    ambitDisplay->requiresHRBelt = requiresHRBelt;

    if (type == MOVESCOUNT_BAROGRAPH_DISPLAY_TYPE) {
        toAmbitBarographDisplay(ambitDisplay);
    }
    else if (type == MOVESCOUNT_LINEGRAPH_DISPLAY_TYPE) {
        toAmbitLinegraphDisplay(ambitDisplay);
    }
    else {
        toAmbitTextDisplay(ambitDisplay);
    }
}

void CustomModeDisplay::toAmbitBarographDisplay(ambit_sport_mode_display_t *ambitDisplay)
{
    ambitDisplay->row1 = 0x07;  // pressure
    ambitDisplay->row2 = 0x0e;
    ambitDisplay->row3 = 0x00;

    libambit_malloc_sport_mode_view(3, ambitDisplay);
    ambitDisplay->view[0] = 0x0d;   // Temperature
    ambitDisplay->view[1] = 0x01;   // Day time
    ambitDisplay->view[2] = 0x12;   // Ref hight
}

void CustomModeDisplay::toAmbitLinegraphDisplay(ambit_sport_mode_display_t *ambitDisplay)
{
    ambitDisplay->row1 = movescount2ambitConverter.value(row1);
    switch (row1) {
    case 0:
        ambitDisplay->row2 = 0x20;
        break;
    case 11:
        ambitDisplay->row2 = 0x16;
        break;
    default:
        ambitDisplay->row2 = 0x6d;
        break;
    }

    ambitDisplay->row3 = 0x05; // Chrono
}

void CustomModeDisplay::toAmbitTextDisplay(ambit_sport_mode_display_t *ambitDisplay)
{
    ambitDisplay->row1 = movescount2ambitConverter.value(row1);
    ambitDisplay->row2 = movescount2ambitConverter.value(row2);
    ambitDisplay->row3 = 0;

    if (libambit_malloc_sport_mode_view(views.count(), ambitDisplay)) {
        uint16_t *ambitViews = ambitDisplay->view;

        foreach (int view, views) {
            *ambitViews = movescount2ambitConverter.value(view);
            ambitViews++;
        }
    }
}

u_int16_t CustomModeDisplay::ambitDisplayType()
{
    switch (type) {
    case MOVESCOUNT_TRIPLE_ROW_DISPLAY_TYPE:
        return AMBIT_TRIPLE_ROW_DISPLAY_TYPE;
    case MOVESCOUNT_DOUBLE_ROWS_DISPLAY_TYPE:
        return AMBIT_DOUBLE_ROWS_DISPLAY_TYPE;
    case MOVESCOUNT_BAROGRAPH_DISPLAY_TYPE:
        return AMBIT_GRAPH_DISPLAY_TYPE;
    case MOVESCOUNT_LINEGRAPH_DISPLAY_TYPE:
        return AMBIT_GRAPH_DISPLAY_TYPE;
    case MOVESCOUNT_SINGLE_ROW_DISPLAY_TYPE:
        return AMBIT_SINGLE_ROW_DISPLAY_TYPE;
    default:
        break;
    }

    return 0;
}

QMap<int, int> qmapInit() {
    QMap<int, int> map;
    map.insert(17, 0xb);    // Speed
    map.insert(3, 0xc);
    map.insert(15, 0x31);
    map.insert(16, 0x1c);   // Pace
    map.insert(25, 0x1d);
    map.insert(4, 0x7);
    map.insert(7, 0x5);     // Chrono
    map.insert(10, 0x1b);
    map.insert(19, 0xd);    // Temperature
    map.insert(20, 0x1);    // Day time
    map.insert(31, 0x30);   // Lap avg pace
    map.insert(12, 0x2d);
    map.insert(13, 0x2e);   // Lap time
    map.insert(23, 0xfffe);
    map.insert(32, 0x44);   // Battery charge
    map.insert(60, 0x43);
    map.insert(8, 0xa);     // Distance
    map.insert(14, 0x2f);   // Lap distance
    map.insert(61, 0x4a);
    map.insert(62, 0x4b);
    map.insert(63, 0x4c);
    map.insert(64, 0x4d);
    map.insert(65, 0x4e);
    map.insert(66, 0x4f);
    map.insert(67, 0x50);
    map.insert(11, 0x15);   // Heart beat
    map.insert(2, 0x9);     // Average heart rate
    map.insert(5, 0x17);
    map.insert(21, 0x1f);   // Peak Training Effect
    map.insert(6, 0x11);    // Calories
    map.insert(0, 0x6);     // Altitude
    map.insert(1, 0x21);
    map.insert(9, 0x22);
    map.insert(22, 0x2c);
    map.insert(33, 0x46);   // Current activity duration
    map.insert(34, 0x47);
    map.insert(35, 0x48);
    map.insert(36, 0x49);
    map.insert(100,0x33); // App index
    map.insert(101,0x34); // App index
    map.insert(102,0x35); // App index
    map.insert(103,0x6b); // App index
    map.insert(104,0x6c); // App index
    map.insert(-1, 0x0);

    return map;
}

//-----------------------------------------------------------------------------
//   objects.h
//
//   Project: EPA SWMM5
//   Version: 5.1
//   Date:    03/19/14  (Build 5.1.000)
//            09/15/14  (Build 5.1.007)
//            03/19/15  (Build 5.1.008)
//            08/05/15  (Build 5.1.010)
//   Author:  L. Rossman (EPA)
//            M. Tryby (EPA)
//            R. Dickinson (CDM)
//
//   Definitions of data structures.
//
//   Most SWMM 5 "objects" are represented as C data structures.
//
//   The units shown next to each structure's properties are SWMM's
//   internal units and may be different than the units required
//   for the property as it appears in the input file.
//
//   In many structure definitions, a blank line separates the set of
//   input properties from the set of computed output properties.
//
//   Build 5.1.007:
//   - Data structure for monthly adjustments of temperature, evaporation,
//     and rainfall added.
//   - User-supplied equation for deep GW flow added to subcatchment object.
//   - Exfiltration added to storage node object.
//   - Surcharge option added to weir object.
//
//   Build 5.1.008:
//   - Route to subcatchment option added to Outfall data structure.
//   - Hydraulic conductivity added to monthly adjustments data structure.
//   - Total LID drain flow and outfall runon added to Runoff Totals.
//   - Groundwater statistics object added.
//   - Maximum depth for reporting times added to node statistics object.
//
//   Build 5.1.010:
//   - Additional fields added to Weir object to support ROADWAY_WEIR type.
//   - New field added to Link object to record when its setting was changed.
//   - q1Old and q2Old of Link object restored.
//-----------------------------------------------------------------------------

#ifndef OBJECTS_H
#define OBJECTS_H

#include "mathexpr.h"
#include "infil.h"
#include "exfil.h"                                                             //(5.1.007)
#include "lid.h"


//-----------------------------------------------------------------------------
// Data Structures
//-----------------------------------------------------------------------------
// enum FileTypes { BINARY, TEXT };         // File mode types

struct TUHData                        // Data for a single unit hydrograph
{                                      // -------------------------------------
	double*   pastRain;                 // array of past rainfall values
	char*     pastMonth;                // month in which past rainfall occurred
	int       period;                   // current UH time period
	int       hasPastRain;              // true if > 0 past periods with rain
	int       maxPeriods;               // max. past rainfall periods
	long      drySeconds;               // time since last nonzero rainfall
	double    iaUsed;                   // initial abstraction used (in or mm)
};

typedef struct TUHData TUHData;

struct TUHGroup                        // Data for a unit hydrograph group
{                                      //---------------------------------
	int       isUsed;                   // true if UH group used by any nodes
	int       rainInterval;             // time interval for RDII processing (sec)
	double    area;                     // sewered area covered by UH's gage (ft2)
	double    rdii;                     // rdii flow (in rainfall units)
	DateTime  gageDate;                 // calendar date of rain gage period
	DateTime  lastDate;                 // date of last rdii computed
	TUHData   uh[3];                    // data for each unit hydrograph
};

typedef struct TUHGroup TUHGroup;

//-----------------------------------------------------------------------------
//  Data Structures - moved from lid.c
//-----------------------------------------------------------------------------

// LID List - list of LID units contained in an LID group
struct LidList
{
	TLidUnit*        lidUnit;     // ptr. to a LID unit
	struct LidList*  nextLidUnit;
};
typedef struct LidList TLidList;

// LID Group - collection of LID units applied to a specific subcatchment
////  Re-defined for release 5.1.008. ////                                     //(5.1.008)
struct LidGroup
{
	double         pervArea;      // amount of pervious area in group (ft2)
	double         flowToPerv;    // total flow sent to pervious area (cfs)
	double         oldDrainFlow;  // total drain flow in previous period (cfs)
	double         newDrainFlow;  // total drain flow in current period (cfs)
	TLidList*      lidList;       // list of LID units in the group
};
typedef struct LidGroup* TLidGroup;


//-----------------------------------------------------------------------------
//  Data Structures
//-----------------------------------------------------------------------------
struct TXnode
{
	char    converged;                 // TRUE if iterations for a node done
	double  newSurfArea;               // current surface area (ft2)
	double  oldSurfArea;               // previous surface area (ft2)
	double  sumdqdh;                   // sum of dqdh from adjoining links
	double  dYdT;                      // change in depth w.r.t. time (ft/sec)
};
typedef struct TXnode TXnode;

//-----------------------------------------------------------------------------                  
// Data Structures - moved from controls
//-----------------------------------------------------------------------------
// Rule Premise Variable
struct TVariable
{
	int      node;            // index of a node (-1 if N/A)
	int      link;            // index of a link (-1 if N/A)
	int      attribute;       // type of attribute for node/link
};

typedef struct TVariable TVariable;

// Rule Premise Clause 
struct  TPremise
{
	int     type;                 // clause type (IF/AND/OR)
	struct  TVariable lhsVar;     // left hand side variable                   //(5.1.008)
	struct  TVariable rhsVar;     // right hand side variable                  //(5.1.008)
	int     relation;             // relational operator (>, <, =, etc)
	double  value;                // right hand side value
	struct  TPremise *next;       // next premise clause of rule
};
typedef struct TPremise TPremise;

// Rule Action Clause
struct  TAction
{
	int     rule;             // index of rule that action belongs to
	int     link;             // index of link being controlled
	int     attribute;        // attribute of link being controlled
	int     curve;            // index of curve for modulated control
	int     tseries;          // index of time series for modulated control
	double  value;            // control setting for link attribute
	double  kp, ki, kd;       // coeffs. for PID modulated control
	double  e1, e2;           // PID set point error from previous time steps
	struct  TAction *next;    // next action clause of rule
};
typedef struct TAction TAction;

// List of Control Actions
struct  TActionList
{
	struct  TAction* action;
	struct  TActionList* next;
};
typedef struct TActionList TActionList;

// Control Rule
struct  TRule
{
	char*    ID;                        // rule ID
	double   priority;                  // priority level
	struct   TPremise* firstPremise;    // pointer to first premise of rule
	struct   TPremise* lastPremise;     // pointer to last premise of rule
	struct   TAction*  thenActions;     // linked list of actions if true
	struct   TAction*  elseActions;     // linked list of actions if false
};
typedef struct TRule TRule;


//-----------------------------------------------------------------------------
//  Data Structures - moved from climate.c
//-----------------------------------------------------------------------------

struct TMovAve
{
	double    tAve;          // moving avg. for daily temperature (deg F)
	double    tRng;          // moving avg. for daily temp. range (deg F)
	double    ta[7];         // data window for tAve
	double    tr[7];         // data window for tRng
	int       count;         // length of moving average window
	int       maxCount;      // maximum length of moving average window
	int       front;         // index of front of moving average window
};

typedef struct TMovAve TMovAve;

//-----------------
// FILE INFORMATION
//-----------------
struct TFile
{
   char          name[MAXFNAME+1];     // file name
   char          mode;                 // NO_FILE, SCRATCH, USE, or SAVE
   char          state;                // current state (OPENED, CLOSED)
   FILE*         file;                 // FILE structure pointer
};

typedef struct TFile TFile;

//-----------------------------------------
// LINKED LIST ENTRY FOR TABLES/TIME SERIES
//-----------------------------------------
struct  TableEntry
{
   double  x;
   double  y;
   struct  TableEntry* next;
};
typedef struct TableEntry TTableEntry;


//-------------------------
// CURVE/TIME SERIES OBJECT
//-------------------------
struct TTable
{
   char*         ID;              // Table/time series ID
   int           curveType;       // type of curve tabulated
   int           refersTo;        // reference to some other object
   double        dxMin;           // smallest x-value interval
   double        lastDate;        // last input date for time series
   double        x1, x2;          // current bracket on x-values
   double        y1, y2;          // current bracket on y-values
   TTableEntry*  firstEntry;      // first data point
   TTableEntry*  lastEntry;       // last data point
   TTableEntry*  thisEntry;       // current data point
   TFile         file;            // external data file
};
typedef struct TTable TTable;


//-----------------
// RAIN GAGE OBJECT
//----------------- 
struct TGage
{
   char*         ID;              // raingage name
   int           dataSource;      // data from time series or file 
   int           tSeries;         // rainfall data time series index
   char          fname[MAXFNAME+1]; // name of rainfall data file
   char          staID[MAXMSG+1]; // station number
   DateTime      startFileDate;   // starting date of data read from file
   DateTime      endFileDate;     // ending date of data read from file
   int           rainType;        // intensity, volume, cumulative
   int           rainInterval;    // recording time interval (seconds)
   int           rainUnits;       // rain depth units (US or SI)
   double        snowFactor;      // snow catch deficiency correction
   //-----------------------------
   long          startFilePos;    // starting byte position in Rain file
   long          endFilePos;      // ending byte position in Rain file
   long          currentFilePos;  // current byte position in Rain file
   double        rainAccum;       // cumulative rainfall
   double        unitsFactor;     // units conversion factor (to inches or mm)
   DateTime      startDate;       // start date of current rainfall
   DateTime      endDate;         // end date of current rainfall
   DateTime      nextDate;        // next date with recorded rainfall
   double        rainfall;        // current rainfall (in/hr or mm/hr)
   double        nextRainfall;    // next rainfall (in/hr or mm/hr)
   double        reportRainfall;  // rainfall value used for reported results
   int           coGage;          // index of gage with same rain timeseries
   int           isUsed;          // TRUE if gage used by any subcatchment
   int           isCurrent;       // TRUE if gage's rainfall is current 
};
typedef struct TGage TGage;


//-------------------
// TEMPERATURE OBJECT
//-------------------
struct TTemp
{
   int           dataSource;      // data from time series or file 
   int           tSeries;         // temperature data time series index
   DateTime      fileStartDate;   // starting date of data read from file
   double        elev;            // elev. of study area (ft)
   double        anglat;          // latitude (degrees)
   double        dtlong;          // longitude correction (hours)
   //-----------------------------
   double        ta;              // air temperature (deg F)
   double        tmax;            // previous day's max. temp. (deg F)
   double        ea;              // saturation vapor pressure (in Hg)
   double        gamma;           // psychrometric constant
   double        tanAnglat;       // tangent of latitude angle
}  ;
typedef struct TTemp TTemp;


//-----------------
// WINDSPEED OBJECT
//-----------------
struct TWind
{
   int          type;             // monthly or file data
   double       aws[12];          // monthly avg. wind speed (mph)
   //-----------------------------
   double        ws;              // wind speed (mph)
}  ;
typedef struct TWind TWind;


//------------
// SNOW OBJECT
//------------
struct TSnow
{
   double        snotmp;          // temp. dividing rain from snow (deg F)
   double        tipm;            // antecedent temp. index parameter
   double        rnm;             // ratio of neg. melt to melt coeff.
   double        adc[2][10];      // areal depletion curves
   //-----------------------------
   double        season;          // snowmelt season
   double        removed;         // total snow plowed out of system (ft3)
}  ;
typedef struct TSnow TSnow;


//-------------------
// EVAPORATION OBJECT
//-------------------
struct TEvap
{
    int          type;            // type of evaporation data
    int          tSeries;         // time series index
    double       monthlyEvap[12]; // monthly evaporation values
    double       panCoeff[12];    // monthly pan coeff. values
    int          recoveryPattern; // soil recovery factor pattern 
    int          dryOnly;         // true if evaporation only in dry periods
    //----------------------------
    double       rate;            // current evaporation rate (ft/sec)
    double       recoveryFactor;  // current soil recovery factor 
}   ;
typedef struct TEvap TEvap;

////  Added for release 5.1.007  ////                                          //(5.1.007)
//-------------------
// ADJUSTMENTS OBJECT
//-------------------
struct TAdjust
{
    double       temp[12];        // monthly temperature adjustments (deg F)
    double       evap[12];        // monthly evaporation adjustments (ft/s)
    double       rain[12];        // monthly rainfall adjustment multipliers
    double       hydcon[12];      // hyd. conductivity adjustment multipliers  //(5.1.008)
    //----------------------------
    double       rainFactor;      // current rainfall adjustment multiplier
    double       hydconFactor;    // current conductivity multiplier           //(5.1.008)
}   ;
typedef struct TAdjust TAdjust;


//-------------------
// AQUIFER OBJECT
//-------------------
struct TAquifer
{
    char*       ID;               // aquifer name
    double      porosity;         // soil porosity
    double      wiltingPoint;     // soil wilting point
    double      fieldCapacity;    // soil field capacity
    double      conductivity;     // soil hyd. conductivity (ft/sec)
    double      conductSlope;     // slope of conductivity v. moisture curve
    double      tensionSlope;     // slope of tension v. moisture curve
    double      upperEvapFrac;    // evaporation available in upper zone
    double      lowerEvapDepth;   // evap depth existing in lower zone (ft)
    double      lowerLossCoeff;   // coeff. for losses to deep GW (ft/sec)
    double      bottomElev;       // elevation of bottom of aquifer (ft)
    double      waterTableElev;   // initial water table elevation (ft)
    double      upperMoisture;    // initial moisture content of unsat. zone
    int         upperEvapPat;     // monthly upper evap. adjustment factors
};
typedef struct TAquifer TAquifer;


////  Added to release 5.1.008.  ////                                          //(5.1.008)
//-----------------------
// GROUNDWATER STATISTICS
//-----------------------
struct TGWaterStats
{
    double       infil;           // total infiltration (ft)
    double       evap;            // total evaporation (ft)
    double       latFlow;         // total lateral outflow (ft)
    double       deepFlow;        // total flow to deep aquifer (ft)
    double       avgUpperMoist;   // avg. upper zone moisture
    double       finalUpperMoist; // final upper zone moisture
    double       avgWaterTable;   // avg. water table height (ft)
    double       finalWaterTable; // final water table height (ft)
    double       maxFlow;         // max. lateral outflow (cfs)
}  ;
typedef struct TGWaterStats TGWaterStats;


//------------------------
// GROUNDWATER OBJECT
//------------------------
struct TGroundwater
{
    int           aquifer;        // index of associated gw aquifer 
    int           node;           // index of node receiving gw flow
    double        surfElev;       // elevation of ground surface (ft)
    double        a1, b1;         // ground water outflow coeff. & exponent
    double        a2, b2;         // surface water outflow coeff. & exponent
    double        a3;             // surf./ground water interaction coeff.
    double        fixedDepth;     // fixed surface water water depth (ft)
    double        nodeElev;       // elevation of receiving node invert (ft)
    double        bottomElev;     // bottom elevation of lower GW zone (ft)
    double        waterTableElev; // initial water table elevation (ft)
    double        upperMoisture;  // initial moisture content of unsat. zone
    //----------------------------
    double        theta;          // upper zone moisture content
    double        lowerDepth;     // depth of saturated zone (ft)
    double        oldFlow;        // gw outflow from previous time period (cfs)
    double        newFlow;        // gw outflow from current time period (cfs)
    double        evapLoss;       // evaporation loss rate (ft/sec)
    double        maxInfilVol;    // max. infil. upper zone can accept (ft)
    TGWaterStats  stats;          // gw statistics                             //(5.1.008)
} ;
typedef struct TGroundwater TGroundwater;


//----------------
// SNOWMELT OBJECT
//----------------
// Snowmelt objects contain parameters that describe the melting
// process of snow packs on 3 different types of surfaces:
//   1 - plowable impervious area
//   2 - non-plowable impervious area
//   3 - pervious area
struct TSnowmelt
{
   char*         ID;              // snowmelt parameter set name
   double        snn;             // fraction of impervious area plowable
   double        si[3];           // snow depth for 100% cover
   double        dhmin[3];        // min. melt coeff. for each surface (ft/sec-F)
   double        dhmax[3];        // max. melt coeff. for each surface (ft/sec-F)
   double        tbase[3];        // base temp. for melting (F)
   double        fwfrac[3];       // free water capacity / snow depth
   double        wsnow[3];        // initial snow depth on each surface (ft)
   double        fwnow[3];        // initial free water in snow pack (ft)
   double        weplow;          // depth at which plowing begins (ft)
   double        sfrac[5];        // fractions moved to other areas by plowing
   int           toSubcatch;      // index of subcatch receiving plowed snow
   //-----------------------------
   double        dhm[3];          // melt coeff. for each surface (ft/sec-F)
}  ;
typedef struct TSnowmelt TSnowmelt;


//----------------
// SNOWPACK OBJECT
//----------------
// Snowpack objects describe the state of the snow melt process on each
// of 3 types of snow surfaces.
struct TSnowpack
{
   int           snowmeltIndex;   // index of snow melt parameter set
   double        fArea[3];        // fraction of total area of each surface
   double        wsnow[3];        // depth of snow pack (ft)
   double        fw[3];           // depth of free water in snow pack (ft)
   double        coldc[3];        // cold content of snow pack
   double        ati[3];          // antecedent temperature index (deg F)
   double        sba[3];          // initial ASC of linear ADC
   double        awe[3];          // initial AWESI of linear ADC
   double        sbws[3];         // final AWESI of linear ADC
   double        imelt[3];        // immediate melt (ft)
};
typedef struct TSnowpack TSnowpack;


//---------------
// SUBAREA OBJECT
//---------------
// An array of 3 subarea objects is associated with each subcatchment object.
// They describe the runoff process on 3 types of surfaces:
//   1 - impervious with no depression storage
//   2 - impervious with depression storage
//   3 - pervious
struct TSubarea
{
   int           routeTo;         // code indicating where outflow is sent
   double        fOutlet;         // fraction of outflow to outlet
   double        N;               // Manning's n
   double        fArea;           // fraction of total area
   double        dStore;          // depression storage (ft)
   //-----------------------------
   double        alpha;           // overland flow factor
   double        inflow;          // inflow rate (ft/sec)
   double        runoff;          // runoff rate (ft/sec)
   double        depth;           // depth of surface runoff (ft)
}  ;
typedef struct TSubarea TSubarea;


//-------------------------
// LAND AREA LANDUSE FACTOR
//-------------------------
struct TLandFactor
{
   double        fraction;        // fraction of land area with land use
   double*       buildup;         // array of buildups for each pollutant
   DateTime      lastSwept;       // date/time of last street sweeping
}  ;
typedef struct TLandFactor TLandFactor;


//--------------------
// SUBCATCHMENT OBJECT
//--------------------
struct TSubcatch
{
   char*         ID;              // subcatchment name
   char          rptFlag;         // reporting flag
   int           gage;            // raingage index
   int           outNode;         // outlet node index
   int           outSubcatch;     // outlet subcatchment index
   int           infil;           // infiltration object index
   TSubarea      subArea[3];      // sub-area data
   double        width;           // overland flow width (ft)
   double        area;            // area (ft2)
   double        fracImperv;      // fraction impervious
   double        slope;           // slope (ft/ft)
   double        curbLength;      // total curb length (ft)
   double*       initBuildup;     // initial pollutant buildup (mass/ft2)
   TLandFactor*  landFactor;      // array of land use factors
   TGroundwater* groundwater;     // associated groundwater data
   MathExpr*     gwLatFlowExpr;   // user-supplied lateral outflow expression  //(5.1.007)
   MathExpr*     gwDeepFlowExpr;  // user-supplied deep percolation expression //(5.1.007)
   TSnowpack*    snowpack;        // associated snow pack data
   //-----------------------------
   double        lidArea;         // area devoted to LIDs (ft2)
   double        rainfall;        // current rainfall (ft/sec)
   double        evapLoss;        // current evap losses (ft/sec)
   double        infilLoss;       // current infil losses (ft/sec) 
   double        runon;           // runon from other subcatchments (cfs)
   double        oldRunoff;       // previous runoff (cfs)
   double        newRunoff;       // current runoff (cfs)
   double        oldSnowDepth;    // previous snow depth (ft)
   double        newSnowDepth;    // current snow depth (ft)
   double*       oldQual;         // previous runoff quality (mass/L)
   double*       newQual;         // current runoff quality (mass/L)
   double*       pondedQual;      // ponded surface water quality (mass)
   double*       totalLoad;       // total washoff load (lbs or kg)
}  ;
typedef struct TSubcatch TSubcatch;


//-----------------------
// TIME PATTERN DATA
//-----------------------
struct TPattern
{
   char*        ID;               // time pattern name
   int          type;             // time pattern type code
   int          count;            // number of factors
   double       factor[24];       // time pattern factors
}  ;
typedef struct TPattern TPattern;


//------------------------------
// DIRECT EXTERNAL INFLOW OBJECT
//------------------------------
struct ExtInflow
{
   int            param;         // pollutant index (flow = -1)
   int            type;          // CONCEN or MASS
   int            tSeries;       // index of inflow time series
   int            basePat;       // baseline time pattern
   double         cFactor;       // units conversion factor for mass inflow
   double         baseline;      // constant baseline value
   double         sFactor;       // time series scaling factor
   struct ExtInflow* next;       // pointer to next inflow data object
};
typedef struct ExtInflow TExtInflow;


//-------------------------------
// DRY WEATHER FLOW INFLOW OBJECT
//-------------------------------
struct DwfInflow
{
   int            param;          // pollutant index (flow = -1)
   double         avgValue;       // average value (cfs or concen.)
   int            patterns[4];    // monthly, daily, hourly, weekend time patterns
   struct DwfInflow* next;        // pointer to next inflow data object
};
typedef struct DwfInflow TDwfInflow;


//-------------------
// RDII INFLOW OBJECT
//-------------------
struct TRdiiInflow
{
   int           unitHyd;         // index of unit hydrograph
   double        area;            // area of sewershed (ft2)
}  ;
typedef struct TRdiiInflow TRdiiInflow;


//-----------------------------
// UNIT HYDROGRAPH GROUP OBJECT
//-----------------------------
struct TUnitHyd
{
   char*         ID;              // name of the unit hydrograph object
   int           rainGage;        // index of rain gage
   double        iaMax[12][3];    // max. initial abstraction (IA) (in or mm)
   double        iaRecov[12][3];  // IA recovery rate (in/day or mm/day)
   double        iaInit[12][3];   // starting IA (in or mm)
   double        r[12][3];        // fraction of rainfall becoming I&I
   long          tBase[12][3];    // time base of each UH in each month (sec)
   long          tPeak[12][3];    // time to peak of each UH in each month (sec)
};
typedef struct TUnitHyd TUnitHyd;


//-----------------
// TREATMENT OBJECT
//-----------------
struct TTreatment
{
    int          treatType;       // treatment equation type: REMOVAL/CONCEN
    MathExpr*    equation;        // treatment eqn. as tokenized math terms
};
typedef struct TTreatment TTreatment;


//------------
// NODE OBJECT
//------------
struct TNode
{
   char*         ID;              // node ID
   int           type;            // node type code
   int           subIndex;        // index of node's sub-category
   char          rptFlag;         // reporting flag
   double        invertElev;      // invert elevation (ft)
   double        initDepth;       // initial storage level (ft)
   double        fullDepth;       // dist. from invert to surface (ft)
   double        surDepth;        // added depth under surcharge (ft)
   double        pondedArea;      // area filled by ponded water (ft2)
   TExtInflow*   extInflow;       // pointer to external inflow data
   TDwfInflow*   dwfInflow;       // pointer to dry weather flow inflow data
   TRdiiInflow*  rdiiInflow;      // pointer to RDII inflow data
   TTreatment*   treatment;       // array of treatment data
   //-----------------------------
   int           degree;          // number of outflow links
   char          updated;         // true if state has been updated
   double        crownElev;       // top of highest connecting conduit (ft)
   double        inflow;          // total inflow (cfs)
   double        outflow;         // total outflow (cfs)
   double        losses;          // evap + exfiltration loss (ft3);           //(5.1.007)
   double        oldVolume;       // previous volume (ft3)
   double        newVolume;       // current volume (ft3)
   double        fullVolume;      // max. storage available (ft3)
   double        overflow;        // overflow rate (cfs)
   double        oldDepth;        // previous water depth (ft)
   double        newDepth;        // current water depth (ft)
   double        oldLatFlow;      // previous lateral inflow (cfs)
   double        newLatFlow;      // current lateral inflow (cfs)
   double*       oldQual;         // previous quality state
   double*       newQual;         // current quality state
   double        oldFlowInflow;   // previous flow inflow
   double        oldNetInflow;    // previous net inflow
}  ;
typedef struct TNode TNode;


//---------------
// OUTFALL OBJECT
//---------------
struct TOutfall
{
   int        type;               // outfall type code
   char       hasFlapGate;        // true if contains flap gate
   double     fixedStage;         // fixed outfall stage (ft)
   int        tideCurve;          // index of tidal stage curve
   int        stageSeries;        // index of outfall stage time series
   int        routeTo;            // subcatchment index routed onto            //(5.1.008)
   double     vRouted;            // flow volume routed (ft3)                  //(5.1.008)
   double*    wRouted;            // pollutant load routed (mass)              //(5.1.008)
};
typedef struct TOutfall TOutfall;


//--------------------
// STORAGE UNIT OBJECT
//--------------------
struct TStorage
{
   double      fEvap;             // fraction of evaporation realized
   double      aConst;            // surface area at zero height (ft2)
   double      aCoeff;            // coeff. of area v. height curve
   double      aExpon;            // exponent of area v. height curve
   int         aCurve;            // index of tabulated area v. height curve
   TExfil*     exfil;             // ptr. to exfiltration object               //(5.1.007)
   //-----------------------------
   double      hrt;               // hydraulic residence time (sec)
   double      evapLoss;          // evaporation loss (ft3) 
   double      exfilLoss;         // exfiltration loss (ft3)                   //(5.1.007)
};
typedef struct TStorage TStorage;


//--------------------
// FLOW DIVIDER OBJECT
//--------------------
struct TDivider
{
   int         link;              // index of link with diverted flow
   int         type;              // divider type code
   double      qMin;              // minimum inflow for diversion (cfs)
   double      qMax;              // flow when weir is full (cfs)
   double      dhMax;             // height of weir (ft)
   double      cWeir;             // weir discharge coeff.
   int         flowCurve;         // index of inflow v. diverted flow curve
};
typedef struct TDivider TDivider;


//-----------------------------
// CROSS SECTION DATA STRUCTURE
//-----------------------------
struct TXsect
{
   int           type;            // type code of cross section shape
   int           culvertCode;     // type of culvert (if any)
   int           transect;        // index of transect/shape (if applicable)
   double        yFull;           // depth when full (ft)
   double        wMax;            // width at widest point (ft)
   double        ywMax;           // depth at widest point (ft)
   double        aFull;           // area when full (ft2)
   double        rFull;           // hyd. radius when full (ft)
   double        sFull;           // section factor when full (ft^4/3)
   double        sMax;            // section factor at max. flow (ft^4/3)

   // These variables have different meanings depending on section shape
   double        yBot;            // depth of bottom section
   double        aBot;            // area of bottom section
   double        sBot;            // slope of bottom section
   double        rBot;            // radius of bottom section
};
typedef struct TXsect TXsect;


//--------------------------------------
// CROSS SECTION TRANSECT DATA STRUCTURE
//--------------------------------------
#define  N_TRANSECT_TBL  51       // size of transect geometry tables
struct TTransect
{
    char*        ID;                        // section ID
    double       yFull;                     // depth when full (ft)
    double       aFull;                     // area when full (ft2)
    double       rFull;                     // hyd. radius when full (ft)
    double       wMax;                      // width at widest point (ft)
    double       ywMax;                     // depth at max width (ft) 
    double       sMax;                      // section factor at max. flow (ft^4/3)
    double       aMax;                      // area at max. flow (ft2)
    double       lengthFactor;              // floodplain / channel length 
    //--------------------------------------
    double       roughness;                 // Manning's n
    double       areaTbl[N_TRANSECT_TBL];   // table of area v. depth
    double       hradTbl[N_TRANSECT_TBL];   // table of hyd. radius v. depth
    double       widthTbl[N_TRANSECT_TBL];  // table of top width v. depth
    int          nTbl;                      // size of geometry tables
}   ;
typedef struct TTransect TTransect;


//-------------------------------------
// CUSTOM CROSS SECTION SHAPE STRUCTURE
//-------------------------------------
#define N_SHAPE_TBL  51           // size of shape geometry tables
struct TShape
{
    int          curve;                     // index of shape curve
    int          nTbl;                      // size of geometry tables
    double       aFull;                     // area when full 
    double       rFull;                     // hyd. radius when full
    double       wMax;                      // max. width
    double       sMax;                      // max. section factor
    double       aMax;                      // area at max. section factor
    double       areaTbl[N_SHAPE_TBL];      // table of area v. depth
    double       hradTbl[N_SHAPE_TBL];      // table of hyd. radius v. depth
    double       widthTbl[N_SHAPE_TBL];     // table of top width v. depth
} ;
typedef struct TShape TShape;


//------------
// LINK OBJECT
//------------
struct TLink
{
   char*         ID;              // link ID
   int           type;            // link type code
   int           subIndex;        // index of link's sub-category
   char          rptFlag;         // reporting flag
   int           node1;           // start node index
   int           node2;           // end node index
   double        offset1;         // ht. above start node invert (ft)
   double        offset2;         // ht. above end node invert (ft)
   TXsect        xsect;           // cross section data
   double        q0;              // initial flow (cfs)
   double        qLimit;          // constraint on max. flow (cfs)
   double        cLossInlet;      // inlet loss coeff.
   double        cLossOutlet;     // outlet loss coeff.
   double        cLossAvg;        // avg. loss coeff.
   double        seepRate;        // seepage rate (ft/sec)
   int           hasFlapGate;     // true if flap gate present
   //-----------------------------
   double        oldFlow;         // previous flow rate (cfs)
   double        newFlow;         // current flow rate (cfs)
   double        oldDepth;        // previous flow depth (ft)
   double        newDepth;        // current flow depth (ft)
   double        oldVolume;       // previous flow volume (ft3)
   double        newVolume;       // current flow volume (ft3)
   double        surfArea1;       // upstream surface area (ft2)
   double        surfArea2;       // downstream surface area (ft2)
   double        qFull;           // flow when full (cfs)
   double        setting;         // current control setting
   double        targetSetting;   // target control setting
   double        timeLastSet;     // time when setting was last changed        //(5.1.010)
   double        froude;          // Froude number
   double*       oldQual;         // previous quality state
   double*       newQual;         // current quality state
   double*       totalLoad;       // total quality mass loading
   int           flowClass;       // flow classification
   double        dqdh;            // change in flow w.r.t. head (ft2/sec)
   signed char   direction;       // flow direction flag
   char          bypassed;        // bypass dynwave calc. flag
   char          normalFlow;      // normal flow limited flag
   char          inletControl;    // culvert inlet control flag
}  ;
typedef struct TLink TLink;


//---------------
// CONDUIT OBJECT
//---------------
struct TConduit
{
   double        length;          // conduit length (ft)
   double        roughness;       // Manning's n
   char          barrels;         // number of barrels
   //-----------------------------
   double        modLength;       // modified conduit length (ft)
   double        roughFactor;     // roughness factor for DW routing
   double        slope;           // slope
   double        beta;            // discharge factor
   double        qMax;            // max. flow (cfs)
   double        a1, a2;          // upstream & downstream areas (ft2)
   double        q1, q2;          // upstream & downstream flows per barrel (cfs)
   double        q1Old, q2Old;    // previous values of q1 & q2 (cfs)          //(5.1.010)
   double        evapLossRate;    // evaporation rate (cfs)
   double        seepLossRate;    // seepage rate (cfs)
   char          capacityLimited; // capacity limited flag
   char          superCritical;   // super-critical flow flag
   char          hasLosses;       // local losses flag
   char          fullState;       // determines if either or both ends full    //(5.1.008)
}  ;
typedef struct TConduit TConduit;


//------------
// PUMP OBJECT
//------------
struct TPump
{
   int           type;            // pump type
   int           pumpCurve;       // pump curve table index
   double        initSetting;     // initial speed setting 
   double        yOn;             // startup depth (ft)
   double        yOff;            // shutoff depth (ft)
   double        xMin;            // minimum pt. on pump curve 
   double        xMax;            // maximum pt. on pump curve
}  ;
typedef struct TPump TPump;


//---------------
// ORIFICE OBJECT
//---------------
struct TOrifice
{
   int           type;            // orifice type code
   int           shape;           // orifice shape code
   double        cDisch;          // discharge coeff.
   double        orate;           // time to open/close (sec)
   //-----------------------------
   double        cOrif;           // coeff. for orifice flow (ft^2.5/sec)
   double        hCrit;           // inlet depth where weir flow begins (ft)
   double        cWeir;           // coeff. for weir flow (cfs)
   double        length;          // equivalent length (ft)
   double        surfArea;        // equivalent surface area (ft2)
}  ;
typedef struct TOrifice TOrifice;


//------------
// WEIR OBJECT
//------------
struct TWeir
{
   int           type;            // weir type code
   int           shape;           // weir shape code
   double        cDisch1;         // discharge coeff.
   double        cDisch2;         // discharge coeff. for ends
   double        endCon;          // end contractions
   int           canSurcharge;    // true if weir can surcharge                //(5.1.007)
   double        roadWidth;       // width for ROADWAY weir                    //(5.1.010)
   int           roadSurface;     // road surface material                     //(5.1.010)
   //-----------------------------
   double        cSurcharge;      // orifice coeff. for surcharge              //(5.1.007)
   double        length;          // equivalent length (ft)
   double        slope;           // slope for Vnotch & Trapezoidal weirs
   double        surfArea;        // equivalent surface area (ft2)
}  ;
typedef struct TWeir TWeir;


//---------------------
// OUTLET DEVICE OBJECT
//---------------------
struct TOutlet
{
    double       qCoeff;          // discharge coeff.
    double       qExpon;          // discharge exponent
    int          qCurve;          // index of discharge rating curve
    int          curveType;       // rating curve type
}   ;
typedef struct TOutlet TOutlet;


//-----------------
// POLLUTANT OBJECT
//-----------------
struct TPollut
{
   char*         ID;              // Pollutant ID
   int           units;           // units
   double        mcf;             // mass conversion factor
   double        dwfConcen;       // dry weather sanitary flow concen.
   double        pptConcen;       // precip. concen.
   double        gwConcen;        // groundwater concen.
   double        rdiiConcen;      // RDII concen.
   double        initConcen;      // initial concen. in conveyance network
   double        kDecay;          // decay constant (1/sec)
   int           coPollut;        // co-pollutant index
   double        coFraction;      // co-pollutant fraction
   int           snowOnly;        // TRUE if buildup occurs only under snow
}  ;
typedef struct TPollut TPollut;


//------------------------
// BUILDUP FUNCTION OBJECT
//------------------------
struct TBuildup
{
   int           normalizer;      // normalizer code (area or curb length)
   int           funcType;        // buildup function type code
   double        coeff[3];        // coeffs. of buildup function
   double        maxDays;         // time to reach max. buildup (days)
}  ;
typedef struct TBuildup TBuildup;


//------------------------
// WASHOFF FUNCTION OBJECT
//------------------------
struct TWashoff
{
   int           funcType;        // washoff function type code
   double        coeff;           // function coeff.
   double        expon;           // function exponent
   double        sweepEffic;      // street sweeping fractional removal
   double        bmpEffic;        // best mgt. practice fractional removal
};
typedef struct TWashoff TWashoff;


//---------------
// LANDUSE OBJECT
//---------------
struct TLanduse
{
   char*         ID;              // landuse name
   double        sweepInterval;   // street sweeping interval (days)
   double        sweepRemoval;    // fraction of buildup available for sweeping
   double        sweepDays0;      // days since last sweeping at start
   TBuildup*     buildupFunc;     // array of buildup functions for pollutants
   TWashoff*     washoffFunc;     // array of washoff functions for pollutants
}  ;
typedef struct TLanduse TLanduse;


//--------------------------
// REPORTING FLAGS STRUCTURE
//--------------------------
struct TRptFlags
{
   char          report;          // TRUE if results report generated
   char          input;           // TRUE if input summary included
   char          subcatchments;   // TRUE if subcatchment results reported
   char          nodes;           // TRUE if node results reported
   char          links;           // TRUE if link results reported
   char          continuity;      // TRUE if continuity errors reported
   char          flowStats;       // TRUE if routing link flow stats. reported
   char          nodeStats;       // TRUE if routing node depth stats. reported
   char          controls;        // TRUE if control actions reported
   int           linesPerPage;    // number of lines printed per page
}  ;
typedef struct TRptFlags TRptFlags;


//-------------------------------
// CUMULATIVE RUNOFF TOTALS
//-------------------------------
struct TRunoffTotals
{                                 // All volume totals are in ft3.             //(5.1.008)
   double        rainfall;        // rainfall volume 
   double        evap;            // evaporation loss
   double        infil;           // infiltration loss
   double        runoff;          // runoff volume
   double        drains;          // LID drains                                //(5.1.008)
   double        runon;           // runon from outfalls                       //(5.1.008)
   double        initStorage;     // inital surface storage
   double        finalStorage;    // final surface storage
   double        initSnowCover;   // initial snow cover
   double        finalSnowCover;  // final snow cover
   double        snowRemoved;     // snow removal
   double        pctError;        // continuity error (%)
};
typedef struct TRunoffTotals TRunoffTotals;


//--------------------------
// CUMULATIVE LOADING TOTALS
//--------------------------
struct TLoadingTotals
{                                 // All loading totals are in lbs.
   double        initLoad;        // initial loading
   double        buildup;         // loading added from buildup
   double        deposition;      // loading added from wet deposition
   double        sweeping;        // loading removed by street sweeping
   double        bmpRemoval;      // loading removed by BMPs
   double        infil;           // loading removed by infiltration
   double        runoff;          // loading removed by runoff
   double        finalLoad;       // final loading
   double        pctError;        // continuity error (%)
};
typedef struct TLoadingTotals TLoadingTotals;


//------------------------------
// CUMULATIVE GROUNDWATER TOTALS
//------------------------------
struct TGwaterTotals
{                                 // All GW flux totals are in feet.
   double        infil;           // surface infiltration
   double        upperEvap;       // upper zone evaporation loss
   double        lowerEvap;       // lower zone evaporation loss
   double        lowerPerc;       // percolation out of lower zone
   double        gwater;          // groundwater flow
   double        initStorage;     // initial groundwater storage
   double        finalStorage;    // final groundwater storage
   double        pctError;        // continuity error (%)
};
typedef struct TGwaterTotals TGwaterTotals;


//----------------------------
// CUMULATIVE ROUTING TOTALS
//----------------------------
struct TRoutingTotals
{                                  // All routing totals are in ft3.
   double        dwInflow;         // dry weather inflow
   double        wwInflow;         // wet weather inflow
   double        gwInflow;         // groundwater inflow
   double        iiInflow;         // RDII inflow
   double        exInflow;         // direct inflow
   double        flooding;         // internal flooding
   double        outflow;          // external outflow
   double        evapLoss;         // evaporation loss
   double        seepLoss;         // seepage loss
   double        reacted;          // reaction losses
   double        initStorage;      // initial storage volume
   double        finalStorage;     // final storage volume
   double        pctError;         // continuity error
};
typedef struct TRoutingTotals TRoutingTotals;


//-----------------------
// SYSTEM-WIDE STATISTICS
//-----------------------
struct TSysStats
{
   double        minTimeStep;
   double        maxTimeStep;
   double        avgTimeStep;
   double        avgStepCount;
   double        steadyStateCount;
};
typedef struct TSysStats TSysStats;


//--------------------
// RAINFALL STATISTICS
//--------------------
struct TRainStats
{
   DateTime    startDate;
   DateTime    endDate;
   long        periodsRain;
   long        periodsMissing;
   long        periodsMalfunc;
};
typedef struct TRainStats TRainStats;


//------------------------
// SUBCATCHMENT STATISTICS
//------------------------
struct TSubcatchStats
{
    double       precip;
    double       runon;
    double       evap;
    double       infil;
    double       runoff;
    double       maxFlow;         
};
typedef struct TSubcatchStats TSubcatchStats;

//----------------
// NODE STATISTICS
//----------------
struct TNodeStats
{
   double        avgDepth;
   double        maxDepth;
   DateTime      maxDepthDate;
   double        maxRptDepth;                                                  //(5.1.008)
   double        volFlooded;
   double        timeFlooded;
   double        timeSurcharged;
   double        timeCourantCritical;
   double        totLatFlow;
   double        maxLatFlow;
   double        maxInflow;
   double        maxOverflow;
   double        maxPondedVol;
   DateTime      maxInflowDate;
   DateTime      maxOverflowDate;
};
typedef struct TNodeStats TNodeStats;


//-------------------
// STORAGE STATISTICS
//-------------------
struct TStorageStats
{
   double        initVol;
   double        avgVol;
   double        maxVol;
   double        maxFlow;
   double        evapLosses;
   double        exfilLosses;
   DateTime      maxVolDate;
};
typedef struct TStorageStats TStorageStats;


//-------------------
// OUTFALL STATISTICS
//-------------------
struct TOutfallStats
{
   double       avgFlow;
   double       maxFlow;
   double*      totalLoad;   
   int          totalPeriods;
};
typedef struct TOutfallStats TOutfallStats;


//---------------- 
// PUMP STATISTICS
//----------------
struct TPumpStats
{
   double       utilized;
   double       minFlow;
   double       avgFlow;
   double       maxFlow;
   double       volume;
   double       energy;
   double       offCurveLow;
   double       offCurveHigh;
   int          startUps;
   int          totalPeriods;
};
typedef struct TPumpStats TPumpStats;


//----------------
// LINK STATISTICS
//----------------
struct TLinkStats
{
   double        maxFlow;
   DateTime      maxFlowDate;
   double        maxVeloc;
   //DateTime      maxVelocDate;  //deprecated                                 //(5.1.008)
   double        maxDepth;
   double        timeNormalFlow;
   double        timeInletControl;
   double        timeSurcharged;
   double        timeFullUpstream;
   double        timeFullDnstream;
   double        timeFullFlow;
   double        timeCapacityLimited;
   double        timeInFlowClass[MAX_FLOW_CLASSES];
   double        timeCourantCritical;
   long          flowTurns;
   int           flowTurnSign;
};
typedef struct TLinkStats TLinkStats;


//-------------------------
// MAXIMUM VALUE STATISTICS
//-------------------------
struct TMaxStats
{
   int           objType;         // either NODE or LINK
   int           index;           // node or link index
   double        value;           // value of node or link statistic
}; 
typedef struct TMaxStats TMaxStats;


//------------------
// REPORT FIELD INFO
//------------------
struct TRptField
{
   char          Name[80];        // name of reported variable 
   char          Units[80];       // units of reported variable
   char          Enabled;         // TRUE if appears in report table
   int           Precision;       // number of decimal places when reported
};

typedef struct TRptField TRptField;


#endif
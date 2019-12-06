#ifndef __TZ_INFO_H__
#define __TZ_INFO_H__


#if 0
struct timezone {
	int	tz_minuteswest;	/* minutes west of Greenwich */
	int	tz_dsttime;	/* type of dst correction */
};
#endif

struct tz_info
{
    int index;
    char *time_zone;
    char *tz_str;
    int tz_offset;
};

enum {
	TZ_ENIWETOK_KWAJALEIN,
	TZ_MIDWAY,
	TZ_HAWAII,
	TZ_ALASKA,
	TZ_PACIFIC,
	TZ_ARIZONA,
	TZ_MOUNTAIN,
	TZ_CENTRAL_AMERICA,
	TZ_CENTRAL,
	TZ_MEXICO_CITY,
	TZ_SASKATCHEWAN,
	TZ_BOGOTA_LIMA,
	TZ_EASTERN,
	TZ_INDIANA,
	TZ_ATLANTIC,
	TZ_CARACAS,
	TZ_SANTIAGO,
	TZ_BRASILIA,
	TZ_BUENOS_AIRES,
	TZ_GREENLAND,
	TZ_MID_ATLANTIC,
	TZ_AZORES,
	TZ_CAPE_VERDE_IS,
	TZ_CASABLANCA,
	TZ_GREENWICH_MEAN_TIME,
	TZ_AMSTERDAM,
	TZ_BELGRADE,
	TZ_BRUSSELS,
	TZ_SARAJEVO,
	TZ_WEST_CENTRAL_AFRICA,
	TZ_ATHENS,
	TZ_BUCHAREST,
	TZ_CAIRO,
	TZ_HARARE,
	TZ_HELSINKI,
	TZ_JERUSALEM,
	TZ_BAGHDAD,
	TZ_KUWAIT,
	TZ_MOSCOW,
	TZ_NAIROBI,
	TZ_ABU_DHABI,
	TZ_BAKU,
	TZ_EKATERINGBURG,
	TZ_ISLAMABAD,
	TZ_ALMATY,
	TZ_ASTANA,
	TZ_SRI,
	TZ_BANGKOK,
	TZ_KRASNOYARSK,
	TZ_BEIJING,
	TZ_IRKUTSK,
	TZ_KUALA_LUMPUR,
	TZ_PERTH,
	TZ_TAIPEI,
	TZ_TYKYO,
	TZ_SEOUL,
	TZ_YAKUTSK,
	TZ_BRISBANE,
	TZ_CANBERRA,
	TZ_GUAM,
	TZ_HOBART,
	TZ_VLADIVOSTOK,
	TZ_MAGADAN,
	TZ_AUCKLAND,
	TZ_FIJI,
	TZ_NUKUALOFA,
	/* yfchou added for Dlink */
	TZ_M330,
	TZ_P330,
	TZ_P430,
	TZ_P530,
	TZ_P930,
	TZ_P1030	
};

struct tz_info time_zones[] =
{
	{ TZ_ENIWETOK_KWAJALEIN,  "GMT-1200", "WPT+12", -12*60},
	{ TZ_MIDWAY,              "GMT-1100", "WPT+11", -11*60},
	{ TZ_HAWAII,              "GMT-1000", "WPT+10", -10*60},
	{ TZ_ALASKA,              "GMT-0900", "WPT+09", -9*60 },
	{ TZ_PACIFIC,             "GMT-0800", "WPT+08", -8*60 },
	{ TZ_ARIZONA,             "GMT-0700", "WPT+07", -7*60 },
	{ TZ_MOUNTAIN,            "GMT-0700", "WPT+07", -7*60 },
	{ TZ_CENTRAL_AMERICA,     "GMT-0600", "WPT+06", -6*60 },
	{ TZ_CENTRAL,             "GMT-0600", "WPT+06", -6*60 },
	{ TZ_MEXICO_CITY,         "GMT-0600", "WPT+06", -6*60 },
	/* { TZ_SASKATCHEWAN,        "GMT-0600", "WPT+06", -6*60 }, */
	{ TZ_BOGOTA_LIMA,         "GMT-0500", "WPT+05", -5*60 },
	{ TZ_EASTERN,             "GMT-0500", "WPT+05", -5*60 },
	{ TZ_INDIANA,             "GMT-0500", "WPT+05", -5*60 },
	{ TZ_ATLANTIC,            "GMT-0400", "WPT+04", -4*60 },
	{ TZ_CARACAS,             "GMT-0400", "WPT+04", -4*60 },
	{ TZ_SANTIAGO,            "GMT-0400", "WPT+04", -4*60 },
	{ TZ_BRASILIA,            "GMT-0300", "WPT+03", -3*60 },
	{ TZ_BUENOS_AIRES,        "GMT-0300", "WPT+03", -3*60 },
	{ TZ_GREENLAND,           "GMT-0300", "WPT+03", -3*60 },
	{ TZ_MID_ATLANTIC,        "GMT-0200", "WPT+02", -2*60 },
	{ TZ_AZORES,              "GMT-0100", "WPT+01", -1*60 },
	/* { TZ_CAPE_VERDE_IS,       "GMT-0100", "WPT+01", -1*60 }, */
	{ TZ_CASABLANCA,          "GMT",      "WPT+00", 0     },
	{ TZ_GREENWICH_MEAN_TIME, "GMT",      "WPT+00", 0     },
	/* yfchou added */
	{ TZ_GREENWICH_MEAN_TIME, "GMT",      "WPT+00", 0     },
	{ TZ_AMSTERDAM,           "GMT+0100", "WPT-01", 1*60  },
	{ TZ_BELGRADE,            "GMT+0100", "WPT-01", 1*60  },
	/* yfchou added */
	{ TZ_BELGRADE,          "GMT+0100", "WPT-01", 1*60  },
	{ TZ_BRUSSELS,            "GMT+0100", "WPT-01", 1*60  },
	{ TZ_SARAJEVO,            "GMT+0100", "WPT-01", 1*60  },
	/* yfchou added */
	{ TZ_SARAJEVO,         	  "GMT+0100", "WPT-01", 1*60  },
	/* { TZ_WEST_CENTRAL_AFRICA, "GMT+0200", "WPT-02", 2*60  }, */
	{ TZ_ATHENS,              "GMT+0200", "WPT-02", 2*60  },
	{ TZ_BUCHAREST,           "GMT+0200", "WPT-02", 2*60  },
	{ TZ_CAIRO,               "GMT+0200", "WPT-02", 2*60  },
	{ TZ_HARARE,              "GMT+0200", "WPT-02", 2*60  },
	{ TZ_HELSINKI,            "GMT+0200", "WPT-02", 2*60  },
	{ TZ_JERUSALEM,           "GMT+0200", "WPT-02", 2*60  },
	{ TZ_BAGHDAD,             "GMT+0300", "WPT-03", 3*60  },
	{ TZ_KUWAIT,              "GMT+0300", "WPT-03", 3*60  },
	{ TZ_MOSCOW,              "GMT+0300", "WPT-03", 3*60  },
	/* { TZ_NAIROBI,             "GMT+0300", "WPT-03", 3*60  }, */
	{ TZ_ABU_DHABI,           "GMT+0400", "WPT-04", 4*60  },
	{ TZ_BAKU,                "GMT+0400", "WPT-04", 4*60  },
	{ TZ_EKATERINGBURG,       "GMT+0500", "WPT-05", 5*60  },
	/* { TZ_ISLAMABAD,           "GMT+0500", "WPT-05", 5*60  }, */
	{ TZ_ALMATY,              "GMT+0600", "WPT-06", 6*60  },
	/* { TZ_ASTANA,              "GMT+0600", "WPT-06", 6*60  }, */
	/* { TZ_SRI,                 "GMT+0600", "WPT-06", 6*60  }, */
	{ TZ_BANGKOK,             "GMT+0700", "WPT-07", 7*60  },
	/* { TZ_KRASNOYARSK,         "GMT+0700", "WPT-07", 7*60  }, */
	{ TZ_BEIJING,             "GMT+0800", "WPT-08", 8*60  },
	{ TZ_IRKUTSK,             "GMT+0800", "WPT-08", 8*60  },
	/* { TZ_KUALA_LUMPUR,        "GMT+0800", "WPT-08", 8*60  },
	{ TZ_PERTH,               "GMT+0800", "WPT-08", 8*60  },
	{ TZ_TAIPEI,              "GMT+0800", "WPT-08", 8*60  }, */
	{ TZ_TYKYO,               "GMT+0900", "WPT-09", 9*60  },
	/* { TZ_SEOUL,               "GMT+0900", "WPT-09", 9*60  },
	{ TZ_YAKUTSK,             "GMT+0900", "WPT-09", 9*60  }, */
	{ TZ_BRISBANE,            "GMT+1000", "WPT-10", 10*60 },
	{ TZ_CANBERRA,            "GMT+1000", "WPT-10", 10*60 },
	{ TZ_GUAM,                "GMT+1000", "WPT-10", 10*60 },
	{ TZ_HOBART,              "GMT+1000", "WPT-10", 10*60 },
	/* { TZ_VLADIVOSTOK,         "GMT+1000", "WPT-10", 10*60 }, */
	{ TZ_MAGADAN,             "GMT+1100", "WPT-11", 11*60 },
	{ TZ_AUCKLAND,            "GMT+1200", "WPT-12", 12*60 },
	{ TZ_FIJI,                "GMT+1200", "WPT-12", 12*60 },
	/* { TZ_NUKUALOFA,           "GMT+1300", "WPT-13", 13*60 } */
	/* yfchou added for Dlink */
	{ TZ_M330,                "GMT-0330", "WPT+033", -210 },/*54*/
	{ TZ_P330,                "GMT+0330", "WPT-033", 210 },	/*55*/
	{ TZ_P430,                "GMT+0430", "WPT-043", 270 }, /*56*/
	{ TZ_P530,                "GMT+0530", "WPT-053", 330}, /*57*/
	{ TZ_P930,                "GMT+0930", "WPT-093", 570 },/*58*/
	{ TZ_P1030,               "GMT+1030", "WPT-103", 630 }/*59*/
};

#endif /* __TZ_INFO_H__ */

static struct station_t {
  int  id;
  char name[8];
  double freq;
} stations [] = {
  { 0, "PBS",  199.25},
  { 1, "ARD",   189.25},
  { 2, "ZDF",   471.25},
  { 3, "3SAT",  503.25},
  { 4, "SAT1",  615.25},
  { 5, "RTL",   535.25},
  { 6, "VOX",   647.25},
  { 7, "PRO7",  519.25},
  { 8, "RTLII", 631.25},
  { 9, "Kabel1",663.25},
  {10, "Arte",  575.25},
  {11, "N3",    775.25},
};

#define NUM_STATIONS (sizeof(stations)/sizeof(struct station_t))

uint32_t min(uint32_t a, uint32_t b) {
  if (a < b) {
    return a;
  }
  return b;
}

int getRandomLevel(int maxLevel) {
	static uint32_t y = 2463534242UL;
	y ^= y << 13;
	y ^= y >> 17;
	y ^= y << 5;
	uint32_t temp = y;
	uint32_t level = 1;
	while (((temp >>= 1) & 1) != 0) {
		level++;
	}
  return (int)min(level, maxLevel);
}

int floor_log_2(unsigned int n) {
	int pos = 0;
	if (n >= 1 << 16) { n >>= 16; pos += 16; }
	if (n >= 1 << 8) { n >>=  8; pos +=  8; }
	if (n >= 1 << 4) { n >>=  4; pos +=  4; }
	if (n >= 1 << 2) { n >>=  2; pos +=  2; }
	if (n >= 1 << 1) {           pos +=  1; }
	return ((n == 0) ? (-1) : pos);
}

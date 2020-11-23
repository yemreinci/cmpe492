namespace cmpe492 {

/// convolve matrix inp with win. 
/// inp is n1 by n2, win is nw by nw, res is n1 by n2, nw is odd.
void
conv(int n1, int n2, int nw, float const* inp, float const* win, float* res);

}
#pragma once
template <typename T>
class proj {
    T dataX0, dataY0, dataX1, dataY1;
    int screenX0, screenY0, screenX1, screenY1;
    T mXData2screen;
    T bXData2screen;
    T bXData2screenPlus0p5;
    T mYData2screen;
    T bYData2screen;
    T bYData2screenPlus0p5;
    // transformation:
    // screen = (data-data1)/(data2-data1)*(screen2-screen1)+screen1;
    // screen = data * (screen2-screen1)/(data2-data1) + screen1 - data1 * (screen2-screen1)/(data2-data1)
   public:
    proj(T dataX0, T dataY0, T dataX1, T dataY1, int screenX0, int screenY0, int screenX1, int screenY1)
        : dataX0(dataX0),
          dataY0(dataY0),
          dataX1(dataX1),
          dataY1(dataY1),
          screenX0(screenX0),
          screenY0(screenY0),
          screenX1(screenX1),
          screenY1(screenY1),
          mXData2screen((screenX1 - screenX0) / (dataX1 - dataX0)),
          bXData2screen(screenX0 - dataX0 * (screenX1 - screenX0) / (dataX1 - dataX0)),
          bXData2screenPlus0p5(bXData2screen + (T)0.5),
          mYData2screen((screenY1 - screenY0) / (dataY1 - dataY0)),
          bYData2screen(screenY0 - dataY0 * (screenY1 - screenY0) / (dataY1 - dataY0)),
          bYData2screenPlus0p5(bYData2screen + (T)0.5) {}

    proj() {} 
    //** projects data to screen */
    inline int projX(T x) const {
        return x * mXData2screen + bXData2screenPlus0p5;
    }
    //** projects data to screen */
    inline int projY(T y) const {
        return y * mYData2screen + bYData2screenPlus0p5;
    }

    //* projects screen to data */
    inline T unprojX(int xMouse) const {
        xMouse = std::min(xMouse, std::max(screenX0, screenX1));
        xMouse = std::max(xMouse, std::min(screenX0, screenX1));
        return (xMouse - bXData2screen) / mXData2screen;
    }
    //* projects screen to data */
    inline T unprojY(int yMouse) const {
        yMouse = std::min(yMouse, std::max(screenY0, screenY1));
        yMouse = std::max(yMouse, std::min(screenY0, screenY1));
        return (yMouse - bYData2screen) / mYData2screen;
    }
    inline int getScreenWidth() const {
        return std::abs(screenX1 - screenX0);
    }
    inline int getScreenHeight() const {
        return std::abs(screenY1 - screenY0);
    }
    inline int getScreenX0() const {
        return screenX0;
    }
    inline int getScreenY0() const {
        return screenY0;
    }
    inline int getScreenX1() const {
        return screenX1;
    }
    inline int getScreenY1() const {
        return screenY1;
    }
    inline int getScreenXCenter() const {
        return (screenX0 + screenX1) / 2;
    }
    inline int getScreenYCenter() const {
        return (screenY0 + screenY1) / 2;
    }
    inline T getDataX0() const {
        return dataX0;
    }
    inline T getDataX1() const {
        return dataX1;
    }
    inline T getDataY0() const {
        return dataY0;
    }
    inline T getDataY1() const {
        return dataY1;
    }
};

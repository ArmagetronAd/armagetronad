typedef boost::shared_ptr<shape> ShapePtr;

class shape {
    virtual ~shape();
};

class shapeCircle : public shape {
    tValue::Base() x;
    tValue::Base() y;
    tValue::Base() radius;

public :
    shapeCircle(tValue::Base() _x, tValue::Base() _y, tValue::Base() _radius): x(_x), y(_y), radius(_radius) { };

};

class shapeTriangle : public shape {
    tValue::Base() x1;
    tValue::Base() y1;
    tValue::Base() x2;
    tValue::Base() y2;
    tValue::Base() x3;
    tValue::Base() y3;

public:
    shapeSquare(tValue::Base() _x1, tValue::Base() _y1,
                tValue::Base() _x2, tValue::Base() _y2,
                tValue::Base() _x3, tValue::Base() _y3,
               ):
            x1(_x1), y1(_y1),
            x2(_x2), y2(_y2),
            x3(_x3), y3(_y3)
    {};
};

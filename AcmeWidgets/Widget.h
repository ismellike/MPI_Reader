#pragma once
enum CustomerType { G, I, R };
class Widget
{
public:
	Widget();
	~Widget();
private:
	int widgetNumber;
	int date;
	CustomerType type;
	float amount;
};


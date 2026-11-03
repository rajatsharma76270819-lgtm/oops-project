#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <map>
#include <ctime>
#include <cctype>
#include <cstdio>
#include <cstdlib>

using namespace std;

/* ============================================================
 *  Utility helpers
 * ============================================================ */
namespace util {

string line(char ch, int n) { return string(n, ch); }

string toLower(string s) {
    for (size_t i = 0; i < s.size(); i++) s[i] = (char)tolower((unsigned char)s[i]);
    return s;
}

string trim(const string& s) {
    size_t a = 0, b = s.size();
    while (a < b && isspace((unsigned char)s[a])) a++;
    while (b > a && isspace((unsigned char)s[b - 1])) b--;
    return s.substr(a, b - a);
}

string money(double v) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.2f", v);
    return string(buf);
}

string formatTime(time_t t) {
    char buf[32];
    struct tm* tmv = localtime(&t);
    strftime(buf, sizeof(buf), "%d-%b-%Y %H:%M", tmv);
    return string(buf);
}

} // namespace util

/* ============================================================
 *  Custom exception hierarchy (Exception Handling)
 * ============================================================ */
class CanteenException : public runtime_error {
public:
    explicit CanteenException(const string& msg) : runtime_error(msg) {}
};

class ItemNotFoundException : public CanteenException {
public:
    explicit ItemNotFoundException(int id)
        : CanteenException("Item with ID " + to_string(id) + " not found in the menu.") {}
};

class DuplicateItemException : public CanteenException {
public:
    explicit DuplicateItemException(const string& name)
        : CanteenException("Item \"" + name + "\" already exists in the menu.") {}
};

class OutOfStockException : public CanteenException {
public:
    OutOfStockException(const string& name, int available)
        : CanteenException(available == 0
              ? "\"" + name + "\" is OUT OF STOCK."
              : "Only " + to_string(available) + " unit(s) of \"" + name + "\" left in stock.") {}
};

class OrderNotFoundException : public CanteenException {
public:
    explicit OrderNotFoundException(int no)
        : CanteenException("Order number " + to_string(no) + " not found.") {}
};

class InvalidOperationException : public CanteenException {
public:
    explicit InvalidOperationException(const string& msg) : CanteenException(msg) {}
};

class InvalidInputException : public CanteenException {
public:
    explicit InvalidInputException(const string& msg) : CanteenException(msg) {}
};

/* ============================================================
 *  MenuItem hierarchy (Abstraction, Inheritance, Polymorphism)
 * ============================================================ */
class MenuItem {                         /* abstract base class */
private:
    static int nextId;                   /* static member: auto ID generator */
    int id;
    string name;
    double price;
    int stock;

public:
    MenuItem(const string& itemName, double itemPrice, int itemStock)
        : id(nextId++), name(itemName), price(itemPrice), stock(itemStock) {}
    virtual ~MenuItem() {}               /* virtual destructor */

    int getId() const { return id; }
    const string& getName() const { return name; }
    double getPrice() const { return price; }
    int getStock() const { return stock; }

    void setPrice(double p) { price = p; }
    void addStock(int qty) { stock += qty; }
    void reduceStock(int qty) {
        if (qty > stock) throw OutOfStockException(name, stock);
        stock -= qty;
    }

    /* pure virtual: each food category defines its own values */
    virtual string category() const = 0;
    virtual double taxRate() const = 0;          /* GST in percent */
};

int MenuItem::nextId = 101;

class Beverage : public MenuItem {
public:
    Beverage(const string& n, double p, int s) : MenuItem(n, p, s) {}
    string category() const override { return "Beverage"; }
    double taxRate() const override { return 5.0; }
};

class Snack : public MenuItem {
public:
    Snack(const string& n, double p, int s) : MenuItem(n, p, s) {}
    string category() const override { return "Snack"; }
    double taxRate() const override { return 12.0; }
};

class Meal : public MenuItem {
public:
    Meal(const string& n, double p, int s) : MenuItem(n, p, s) {}
    string category() const override { return "Meal"; }
    double taxRate() const override { return 5.0; }
};

class Dessert : public MenuItem {
public:
    Dessert(const string& n, double p, int s) : MenuItem(n, p, s) {}
    string category() const override { return "Dessert"; }
    double taxRate() const override { return 12.0; }
};

/* Factory: builds the right derived object for the chosen category */
shared_ptr<MenuItem> createItem(int categoryChoice, const string& name, double price, int stock) {
    if (name.empty()) throw InvalidInputException("Item name cannot be empty.");
    if (price <= 0)   throw InvalidInputException("Price must be greater than 0.");
    if (stock < 0)    throw InvalidInputException("Stock cannot be negative.");
    switch (categoryChoice) {
        case 1:  return make_shared<Beverage>(name, price, stock);
        case 2:  return make_shared<Snack>(name, price, stock);
        case 3:  return make_shared<Meal>(name, price, stock);
        default: return make_shared<Dessert>(name, price, stock);
    }
}

/* ============================================================
 *  Menu class (Module 1 logic)
 * ============================================================ */
class Menu {
private:
    vector<shared_ptr<MenuItem> > items;

public:
    const vector<shared_ptr<MenuItem> >& getItems() const { return items; }

    void add(shared_ptr<MenuItem> item) {
        for (size_t i = 0; i < items.size(); i++)
            if (util::toLower(items[i]->getName()) == util::toLower(item->getName()))
                throw DuplicateItemException(item->getName());
        items.push_back(item);
    }

    shared_ptr<MenuItem> findById(int id) const {        /* Linear Search */
        for (size_t i = 0; i < items.size(); i++)
            if (items[i]->getId() == id) return items[i];
        throw ItemNotFoundException(id);
    }

    void remove(int id) {
        for (size_t i = 0; i < items.size(); i++) {
            if (items[i]->getId() == id) {
                items.erase(items.begin() + (long)i);
                return;
            }
        }
        throw ItemNotFoundException(id);
    }
};

/* ============================================================
 *  Order and OrderLine (Module 2 & 3 data)
 * ============================================================ */
struct OrderLine {
    shared_ptr<MenuItem> item;           /* used to restore stock on cancel */
    string name;                         /* snapshot at time of order */
    string category;
    double price;
    double taxRate;
    int qty;

    double lineTotal() const { return price * qty; }
};

enum OrderStatus { PENDING, PAID, CANCELLED };

class Order {
private:
    static int counter;                  /* static member: order numbers */

    int orderNo;
    string customer;
    vector<OrderLine> lines;
    OrderStatus status;
    string payMode;
    time_t orderTime;

public:
    static const double BULK_LIMIT;      /* subtotal needed for discount */
    static const double BULK_DISCOUNT;   /* discount rate */

    explicit Order(const string& customerName)
        : orderNo(counter++), customer(customerName), status(PENDING),
          payMode("-"), orderTime(time(NULL)) {}

    int getOrderNo() const { return orderNo; }
    const string& getCustomer() const { return customer; }
    const vector<OrderLine>& getLines() const { return lines; }
    OrderStatus getStatus() const { return status; }
    const string& getPayMode() const { return payMode; }
    time_t getTime() const { return orderTime; }
    bool isEmpty() const { return lines.empty(); }

    void setTime(time_t t) { orderTime = t; }
    void markPaid(const string& mode) { status = PAID; payMode = mode; }
    void markCancelled() { status = CANCELLED; }

    void addLine(shared_ptr<MenuItem> item, int qty) {
        for (size_t i = 0; i < lines.size(); i++) {      /* merge same item */
            if (lines[i].item->getId() == item->getId()) {
                lines[i].qty += qty;
                return;
            }
        }
        OrderLine l;
        l.item = item;
        l.name = item->getName();
        l.category = item->category();       /* polymorphic calls */
        l.price = item->getPrice();
        l.taxRate = item->taxRate();
        l.qty = qty;
        lines.push_back(l);
    }

    /* ---- billing calculations ---- */
    double subtotal() const {
        double s = 0;
        for (size_t i = 0; i < lines.size(); i++) s += lines[i].lineTotal();
        return s;
    }
    double discountRate() const { return subtotal() >= BULK_LIMIT ? BULK_DISCOUNT : 0.0; }
    double discount() const { return subtotal() * discountRate(); }
    double gst() const {                  /* GST on the discounted amount */
        double t = 0;
        for (size_t i = 0; i < lines.size(); i++)
            t += lines[i].lineTotal() * lines[i].taxRate / 100.0;
        return t * (1.0 - discountRate());
    }
    double total() const { return subtotal() - discount() + gst(); }

    string statusText() const {
        return status == PENDING ? "PENDING" : (status == PAID ? "PAID" : "CANCELLED");
    }

    friend ostream& operator<<(ostream& os, const Order& o);
};

int Order::counter = 5001;
const double Order::BULK_LIMIT = 300.0;
const double Order::BULK_DISCOUNT = 0.10;

/* Operator overloading: print a full bill with cout << order */
ostream& operator<<(ostream& os, const Order& o) {
    os << "\n" << util::line('=', 50) << "\n";
    os << "              COLLEGE CANTEEN - BILL\n";
    os << util::line('=', 50) << "\n";
    os << "Order No   : " << o.orderNo << "\n";
    os << "Customer   : " << o.customer << "\n";
    os << "Date/Time  : " << util::formatTime(o.orderTime) << "\n";
    os << "Payment    : " << o.payMode << "\n";
    os << "Status     : " << o.statusText() << "\n";
    os << util::line('-', 50) << "\n";
    os << left << setw(22) << "Item" << setw(6) << "Qty" << right
       << setw(9) << "Price" << setw(13) << "Amount" << "\n";
    os << util::line('-', 50) << "\n";
    for (size_t i = 0; i < o.lines.size(); i++) {
        const OrderLine& l = o.lines[i];
        os << left << setw(22) << l.name << setw(6) << l.qty << right
           << setw(9) << util::money(l.price) << setw(13) << util::money(l.lineTotal()) << "\n";
    }
    os << util::line('-', 50) << "\n";
    os << left << setw(37) << "Subtotal" << right << setw(13) << util::money(o.subtotal()) << "\n";
    if (o.discountRate() > 0) {
        string label = "Bulk Discount (" + to_string((int)(o.discountRate() * 100)) + "%)";
        os << left << setw(37) << label << right << setw(13) << ("-" + util::money(o.discount())) << "\n";
    }
    os << left << setw(37) << "GST" << right << setw(13) << util::money(o.gst()) << "\n";
    os << util::line('-', 50) << "\n";
    os << left << setw(37) << "GRAND TOTAL (Rs)" << right << setw(13) << util::money(o.total()) << "\n";
    os << util::line('=', 50) << "\n";
    if (o.status == PAID) os << "        Thank you! Enjoy your meal.\n";
    else if (o.status == PENDING) os << "     Payment pending - please pay at counter.\n";
    return os;
}

/* ============================================================
 *  Canteen class: core business logic
 * ============================================================ */
class Canteen {
private:
    Menu menu;
    vector<Order> orders;

public:
    static const int LOW_STOCK_LIMIT = 10;

    Canteen() { loadDefaultMenu(); }

    Menu& getMenu() { return menu; }
    const Menu& getMenu() const { return menu; }
    const vector<Order>& getOrders() const { return orders; }

    void loadDefaultMenu() {
        menu.add(createItem(1, "Tea",             15, 50));
        menu.add(createItem(1, "Coffee",          20, 40));
        menu.add(createItem(1, "Cold Drink",      30, 30));
        menu.add(createItem(2, "Samosa",          15, 60));
        menu.add(createItem(2, "Veg Sandwich",    40, 25));
        menu.add(createItem(2, "Burger",          50, 20));
        menu.add(createItem(2, "French Fries",    60, 8));
        menu.add(createItem(3, "Veg Thali",       80, 30));
        menu.add(createItem(3, "Chole Bhature",   70, 25));
        menu.add(createItem(3, "Masala Dosa",     60, 20));
        menu.add(createItem(4, "Gulab Jamun",     25, 40));
        menu.add(createItem(4, "Ice Cream",       30, 6));
    }

    /* Module 2: add an item to an order being built (reserves stock) */
    void addToOrder(Order& order, int itemId, int qty) {
        shared_ptr<MenuItem> item = menu.findById(itemId);
        item->reduceStock(qty);          /* throws OutOfStockException */
        order.addLine(item, qty);
    }

    Order& submitOrder(const Order& order) {
        orders.push_back(order);
        return orders.back();
    }

    Order& findOrder(int no) {
        for (size_t i = 0; i < orders.size(); i++)
            if (orders[i].getOrderNo() == no) return orders[i];
        throw OrderNotFoundException(no);
    }

    void cancelOrder(int no) {
        Order& o = findOrder(no);
        if (o.getStatus() != PENDING)
            throw InvalidOperationException("Only PENDING orders can be cancelled (this order is "
                                            + o.statusText() + ").");
        for (size_t i = 0; i < o.getLines().size(); i++)       /* restore stock */
            o.getLines()[i].item->addStock(o.getLines()[i].qty);
        o.markCancelled();
    }

    /* Module 3: payment */
    Order& payOrder(int no, const string& mode) {
        Order& o = findOrder(no);
        if (o.getStatus() == CANCELLED)
            throw InvalidOperationException("Order " + to_string(no) + " was cancelled.");
        if (o.getStatus() == PENDING) o.markPaid(mode);
        return o;
    }
};

/* ============================================================
 *  Input handling class
 * ============================================================ */
class Input {
public:
    static string readLine() {
        string s;
        if (!getline(cin, s)) {              /* end of input: quit cleanly */
            cout << "\n";
            exit(0);
        }
        return s;
    }

    static string readText(const string& prompt) {
        cout << prompt;
        return util::trim(readLine());
    }

    static int readInt(const string& prompt, int minVal, int maxVal) {
        while (true) {
            cout << prompt;
            string s = util::trim(readLine());
            try {
                size_t pos = 0;
                int val = stoi(s, &pos);
                if (pos == s.size() && val >= minVal && val <= maxVal) return val;
            } catch (const exception&) { }
            cout << "  [!] Invalid input. Enter a number between "
                 << minVal << " and " << maxVal << ".\n";
        }
    }

    static double readPrice(const string& prompt, double minVal, double maxVal) {
        while (true) {
            cout << prompt;
            string s = util::trim(readLine());
            try {
                size_t pos = 0;
                double val = stod(s, &pos);
                if (pos == s.size() && val >= minVal && val <= maxVal) return val;
            } catch (const exception&) { }
            cout << "  [!] Invalid input. Enter an amount between "
                 << minVal << " and " << maxVal << ".\n";
        }
    }

    static void pressEnter() {
        cout << "\nPress Enter to continue...";
        readLine();
    }
};

/* ============================================================
 *  CanteenApp class: menu and user interface
 * ============================================================ */
class CanteenApp {
private:
    Canteen canteen;

    static string paymentName(int choice) {
        return choice == 1 ? "Cash" : (choice == 2 ? "UPI" : "Card");
    }

    void printMenu() const {
        cout << "\n==================================================\n";
        cout << "            CANTEEN MANAGEMENT SYSTEM\n";
        cout << "==================================================\n";
        cout << " [Module 1: Menu Management]\n";
        cout << "   1. Display Menu\n";
        cout << "   2. Add New Item\n";
        cout << "   3. Update Price / Restock Item\n";
        cout << "   4. Remove Item\n\n";
        cout << " [Module 2: Order Management]\n";
        cout << "   5. Place New Order\n";
        cout << "   6. View All Orders\n";
        cout << "   7. Cancel Pending Order\n\n";
        cout << " [Module 3: Billing]\n";
        cout << "   8. Pay & Generate Bill / Reprint Bill\n\n";
        cout << " [Module 4: Sales Reports]\n";
        cout << "   9. Sales Report (Best Sellers)\n";
        cout << "  10. Low Stock Alert\n\n";
        cout << "  11. Load Demo Orders\n";
        cout << "   0. Exit System\n";
        cout << "==================================================\n";
    }

    /* ---------- Module 1 ---------- */
    void displayMenu() const {
        const vector<shared_ptr<MenuItem> >& items = canteen.getMenu().getItems();
        cout << "\n===== CANTEEN MENU =====\n";
        if (items.empty()) { cout << "[!] Menu is empty.\n"; return; }

        cout << util::line('-', 70) << "\n";
        cout << left << setw(7) << "ID" << setw(22) << "Item" << setw(12) << "Category"
             << right << setw(9) << "Price" << setw(8) << "GST%" << setw(12) << "Stock" << "\n";
        cout << util::line('-', 70) << "\n";
        for (size_t i = 0; i < items.size(); i++) {
            const MenuItem& m = *items[i];
            string stock = m.getStock() == 0 ? "OUT" : to_string(m.getStock());
            cout << left << setw(7) << m.getId() << setw(22) << m.getName() << setw(12) << m.category()
                 << right << setw(9) << util::money(m.getPrice()) << setw(8) << (int)m.taxRate()
                 << setw(12) << stock << "\n";
        }
        cout << util::line('-', 70) << "\n";
    }

    void addItem() {
        cout << "\n===== ADD NEW ITEM =====\n";
        cout << "  1. Beverage (5% GST)\n  2. Snack (12% GST)\n  3. Meal (5% GST)\n  4. Dessert (12% GST)\n";
        int cat = Input::readInt("Select category (1-4): ", 1, 4);
        string name = Input::readText("Enter item name: ");
        double price = Input::readPrice("Enter price (Rs): ", 1, 5000);
        int stock = Input::readInt("Enter opening stock (0-1000): ", 0, 1000);

        shared_ptr<MenuItem> item = createItem(cat, name, price, stock);
        canteen.getMenu().add(item);
        cout << "\n[+] Item added: ID " << item->getId() << " - " << item->getName()
             << " (" << item->category() << ")\n";
    }

    void updateItem() {
        cout << "\n===== UPDATE PRICE / RESTOCK =====\n";
        int id = Input::readInt("Enter Item ID: ", 1, 999999);
        shared_ptr<MenuItem> item = canteen.getMenu().findById(id);
        cout << "Selected: " << item->getName() << " | Price: Rs " << util::money(item->getPrice())
             << " | Stock: " << item->getStock() << "\n";
        cout << "  1. Update price\n  2. Add stock\n";
        int ch = Input::readInt("Choose (1-2): ", 1, 2);
        if (ch == 1) {
            double p = Input::readPrice("Enter new price (Rs): ", 1, 5000);
            item->setPrice(p);
            cout << "\n[+] Price updated to Rs " << util::money(p) << ".\n";
        } else {
            int q = Input::readInt("Enter quantity to add (1-1000): ", 1, 1000);
            item->addStock(q);
            cout << "\n[+] Stock updated. New stock: " << item->getStock() << ".\n";
        }
    }

    void removeItem() {
        cout << "\n===== REMOVE ITEM =====\n";
        int id = Input::readInt("Enter Item ID to remove: ", 1, 999999);
        shared_ptr<MenuItem> item = canteen.getMenu().findById(id);
        string name = item->getName();
        canteen.getMenu().remove(id);
        cout << "\n[+] \"" << name << "\" removed from the menu.\n";
    }

    /* ---------- Module 3 (used by order flow too) ---------- */
    void payAndBill(int orderNo) {
        Order& o = canteen.findOrder(orderNo);
        if (o.getStatus() == CANCELLED)
            throw InvalidOperationException("Order " + to_string(orderNo) + " was cancelled.");

        if (o.getStatus() == PENDING) {
            cout << o;                                   /* show bill before payment */
            cout << "\n  1. Cash\n  2. UPI\n  3. Card\n";
            int m = Input::readInt("Select payment mode (1-3): ", 1, 3);
            canteen.payOrder(orderNo, paymentName(m));
            cout << "\n[+] Payment of Rs " << util::money(o.total()) << " received via "
                 << paymentName(m) << ".\n";
        } else {
            cout << "\n(Order already paid, reprinting bill)\n";
        }
        cout << o;
    }

    /* ---------- Module 2 ---------- */
    void placeOrder() {
        cout << "\n===== PLACE NEW ORDER =====\n";
        string customer = Input::readText("Enter customer name: ");
        if (customer.empty()) customer = "Guest";

        displayMenu();
        Order order(customer);
        cout << "\nEnter items one by one. Enter Item ID 0 when finished.\n";

        while (true) {
            int id = Input::readInt("\nItem ID (0 to finish): ", 0, 999999);
            if (id == 0) break;
            try {
                shared_ptr<MenuItem> item = canteen.getMenu().findById(id);
                int qty = Input::readInt("Quantity for " + item->getName() + ": ", 1, 100);
                canteen.addToOrder(order, id, qty);
                cout << "  [+] Added. Running subtotal: Rs " << util::money(order.subtotal()) << "\n";
            } catch (const CanteenException& e) {
                cout << "  [!] " << e.what() << "\n";
            }
        }

        if (order.isEmpty()) {
            cout << "\n[!] No items selected. Order discarded.\n";
            return;
        }

        Order& placed = canteen.submitOrder(order);
        cout << "\n[+] Order " << placed.getOrderNo() << " placed successfully.\n";
        if (Input::readInt("Proceed to payment and billing now? (1 = Yes, 0 = Later): ", 0, 1) == 1)
            payAndBill(placed.getOrderNo());
        else
            cout << "Order " << placed.getOrderNo() << " is pending. Use option 8 later.\n";
    }

    void viewOrders() const {
        const vector<Order>& orders = canteen.getOrders();
        cout << "\n===== ALL ORDERS =====\n";
        if (orders.empty()) { cout << "[!] No orders yet.\n"; return; }

        cout << util::line('-', 74) << "\n";
        cout << left << setw(9) << "Order" << setw(16) << "Customer" << setw(18) << "Date/Time"
             << setw(7) << "Items" << right << setw(11) << "Total" << "  " << left << setw(10) << "Status" << "\n";
        cout << util::line('-', 74) << "\n";
        for (size_t i = 0; i < orders.size(); i++) {
            int items = 0;
            for (size_t j = 0; j < orders[i].getLines().size(); j++) items += orders[i].getLines()[j].qty;
            cout << left << setw(9) << orders[i].getOrderNo() << setw(16) << orders[i].getCustomer()
                 << setw(18) << util::formatTime(orders[i].getTime()) << setw(7) << items
                 << right << setw(11) << util::money(orders[i].total()) << "  " << left
                 << setw(10) << orders[i].statusText() << "\n";
        }
        cout << util::line('-', 74) << "\n";
    }

    void cancelOrder() {
        cout << "\n===== CANCEL PENDING ORDER =====\n";
        int no = Input::readInt("Enter Order No: ", 1, 999999);
        canteen.cancelOrder(no);
        cout << "\n[+] Order " << no << " cancelled. Stock has been restored.\n";
    }

    void billingMenu() {
        cout << "\n===== PAY & GENERATE BILL =====\n";
        const vector<Order>& orders = canteen.getOrders();
        int pending = 0;
        for (size_t i = 0; i < orders.size(); i++) {
            if (orders[i].getStatus() == PENDING) {
                cout << "  Pending: Order " << orders[i].getOrderNo() << " | " << orders[i].getCustomer()
                     << " | Rs " << util::money(orders[i].total()) << "\n";
                pending++;
            }
        }
        if (pending == 0) cout << "  (No pending orders)\n";
        int no = Input::readInt("\nEnter Order No (0 to cancel): ", 0, 999999);
        if (no == 0) return;
        payAndBill(no);
    }

    /* ---------- Module 4 ---------- */
    struct ItemSale { string name; int qty; double revenue; };

    void salesReport() const {
        const vector<Order>& orders = canteen.getOrders();
        cout << "\n===== SALES REPORT =====\n";

        int paid = 0, pending = 0, cancelled = 0;
        double revenue = 0, gstTotal = 0, discountTotal = 0;
        vector<ItemSale> sales;
        map<string, double> categorySales;

        for (size_t i = 0; i < orders.size(); i++) {
            const Order& o = orders[i];
            if (o.getStatus() == PENDING)   { pending++;   continue; }
            if (o.getStatus() == CANCELLED) { cancelled++; continue; }
            paid++;
            revenue += o.total();
            gstTotal += o.gst();
            discountTotal += o.discount();
            for (size_t j = 0; j < o.getLines().size(); j++) {
                const OrderLine& l = o.getLines()[j];
                categorySales[l.category] += l.lineTotal();
                size_t k;
                for (k = 0; k < sales.size(); k++) {
                    if (sales[k].name == l.name) {
                        sales[k].qty += l.qty;
                        sales[k].revenue += l.lineTotal();
                        break;
                    }
                }
                if (k == sales.size()) {
                    ItemSale s; s.name = l.name; s.qty = l.qty; s.revenue = l.lineTotal();
                    sales.push_back(s);
                }
            }
        }

        cout << "Orders  - Paid: " << paid << " | Pending: " << pending
             << " | Cancelled: " << cancelled << "\n";
        if (paid == 0) { cout << "\n[!] No paid orders yet, so no sales to report.\n"; return; }

        cout << "Total Revenue (incl. GST) : Rs " << util::money(revenue) << "\n";
        cout << "GST Collected             : Rs " << util::money(gstTotal) << "\n";
        cout << "Discounts Given           : Rs " << util::money(discountTotal) << "\n";
        cout << "Average Order Value       : Rs " << util::money(revenue / paid) << "\n";

        sort(sales.begin(), sales.end(), [](const ItemSale& a, const ItemSale& b) {
            return a.qty > b.qty;                        /* best sellers first */
        });

        cout << "\nBest Selling Items:\n" << util::line('-', 46) << "\n";
        cout << left << setw(6) << "Rank" << setw(22) << "Item" << setw(8) << "Sold"
             << right << setw(10) << "Sales(Rs)" << "\n" << util::line('-', 46) << "\n";
        for (size_t i = 0; i < sales.size(); i++)
            cout << left << setw(6) << (i + 1) << setw(22) << sales[i].name << setw(8) << sales[i].qty
                 << right << setw(10) << util::money(sales[i].revenue) << "\n";
        cout << util::line('-', 46) << "\n";
        cout << "TOP SELLER: " << sales[0].name << " (" << sales[0].qty << " sold)\n";

        cout << "\nCategory-wise Sales (before discount & GST):\n";
        for (map<string, double>::const_iterator it = categorySales.begin(); it != categorySales.end(); ++it)
            cout << "  " << left << setw(12) << it->first << "Rs " << util::money(it->second) << "\n";
    }

    void lowStockAlert() const {
        const vector<shared_ptr<MenuItem> >& items = canteen.getMenu().getItems();
        cout << "\n===== LOW STOCK ALERT (" << Canteen::LOW_STOCK_LIMIT << " or fewer) =====\n";
        int count = 0;
        for (size_t i = 0; i < items.size(); i++) {
            if (items[i]->getStock() <= Canteen::LOW_STOCK_LIMIT) {
                cout << "  ID " << items[i]->getId() << " | " << left << setw(20) << items[i]->getName()
                     << " | Stock: " << (items[i]->getStock() == 0 ? string("OUT OF STOCK")
                                                                   : to_string(items[i]->getStock())) << "\n";
                count++;
            }
        }
        if (count == 0) cout << "  All items are sufficiently stocked.\n";
        else cout << "\n" << count << " item(s) need restocking (use option 3).\n";
    }

    /* ---------- Demo data ---------- */
    void loadDemoOrders() {
        struct Entry { int id; int qty; };
        struct Demo { const char* customer; int pay; int count; Entry e[4]; int minsAgo; };
        const Demo demo[] = {
            { "Aman",  1, 2, { {101, 2}, {104, 2}, {0, 0}, {0, 0} },             95 },
            { "Priya", 2, 3, { {108, 2}, {103, 2}, {111, 1}, {0, 0} },           70 },
            { "Rohit", 3, 4, { {109, 3}, {106, 2}, {102, 2}, {112, 2} },         45 },
            { "Neha",  1, 2, { {105, 1}, {101, 1}, {0, 0}, {0, 0} },             25 },
            { "Karan", 0, 1, { {110, 2}, {0, 0}, {0, 0}, {0, 0} },               5 }
        };
        int placed = 0;
        for (size_t i = 0; i < sizeof(demo) / sizeof(demo[0]); i++) {
            try {
                Order o(demo[i].customer);
                o.setTime(time(NULL) - (time_t)demo[i].minsAgo * 60);
                for (int k = 0; k < demo[i].count; k++)
                    canteen.addToOrder(o, demo[i].e[k].id, demo[i].e[k].qty);
                if (o.isEmpty()) continue;
                Order& s = canteen.submitOrder(o);
                if (demo[i].pay > 0) canteen.payOrder(s.getOrderNo(), paymentName(demo[i].pay));
                placed++;
            } catch (const CanteenException&) { /* skip if item missing or out of stock */ }
        }
        cout << "\n[+] " << placed << " demo order(s) loaded (the last one is left PENDING).\n";
    }

public:
    void run() {
        int choice;
        do {
            printMenu();
            choice = Input::readInt("Enter your choice: ", 0, 11);
            try {
                switch (choice) {
                    case 1:  displayMenu();     break;
                    case 2:  addItem();         break;
                    case 3:  updateItem();      break;
                    case 4:  removeItem();      break;
                    case 5:  placeOrder();      break;
                    case 6:  viewOrders();      break;
                    case 7:  cancelOrder();     break;
                    case 8:  billingMenu();     break;
                    case 9:  salesReport();     break;
                    case 10: lowStockAlert();   break;
                    case 11: loadDemoOrders();  break;
                    case 0:  cout << "\nThank you for using the Canteen Management System. Goodbye!\n"; break;
                }
            } catch (const CanteenException& e) {
                cout << "\n[!] " << e.what() << "\n";
            }
            if (choice != 0) Input::pressEnter();
        } while (choice != 0);
    }
};

int main() {
    CanteenApp app;
    app.run();
    return 0;
}

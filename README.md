# Canteen Management System (C++ / OOP)

A terminal-based Canteen Management System developed in C++ using core Object-Oriented Programming concepts: Classes & Objects, Encapsulation, Abstraction, Inheritance, Polymorphism, Exception Handling and Operator Overloading.

---

## 👥 Team Members

| Roll Number | Student Name |
|-------------|--------------|
| `25EGJIT023` | Rajat Sharma |
| `25EGJIT024` | Raksha Parashar |
| `25EGJIT025` | Rohit Kumar Chandel |
| `25EGJIT026` | Tushar Sharma |

---

## 📌 Project Overview & Module Breakdown

The system starts with a ready-made menu of 12 items across 4 categories. The project is divided into 4 modules:

### 1. Menu Management
* **Display Menu:** Shows ID, item name, category, price, GST % and stock (`OUT` when finished).
* **Add New Item:** Choose a category (Beverage, Snack, Meal, Dessert), then enter name, price and opening stock. Duplicate names are rejected.
* **Update Price / Restock:** Change an item's price or add stock by Item ID.
* **Remove Item:** Deletes an item from the menu (old bills keep their data).

### 2. Order Management
* **Place New Order:** Add multiple items with quantities. Stock is checked and reserved immediately; out-of-stock or invalid items are reported without stopping the order.
* **View All Orders:** Table of all orders with customer, time, item count, total and status (`PENDING`, `PAID`, `CANCELLED`).
* **Cancel Pending Order:** Cancels an unpaid order and **restores the stock**.

### 3. Billing
* **Pay & Generate Bill:** Select payment mode (Cash / UPI / Card) and print a detailed bill. Paid bills can be reprinted using the order number.
* **Bulk Discount:** 10% discount when the subtotal is Rs 300 or more.
* **GST:** Calculated per category on the discounted amount.

| Category | GST |
|----------|-----|
| Beverage | 5% |
| Snack | 12% |
| Meal | 5% |
| Dessert | 12% |

### 4. Sales Reports
* **Sales Report:** Paid / pending / cancelled orders, total revenue, GST collected, discounts given and average order value.
* **Best Sellers:** Items ranked by quantity sold (sorted), with the top seller highlighted.
* **Category-wise Sales:** Revenue split by Beverage, Snack, Meal and Dessert.
* **Low Stock Alert:** Lists items with 10 or fewer units left.

---

## 🧱 Class Design

```text
MenuItem (abstract)
 ├── Beverage    (GST 5%)
 ├── Snack       (GST 12%)
 ├── Meal        (GST 5%)
 └── Dessert     (GST 12%)

Menu          : collection of MenuItems (add, find, remove)
OrderLine     : one item + quantity in an order (price snapshot)
Order         : one customer order, bill calculations, prints with operator<<
Canteen       : owns Menu and Orders, core business logic
CanteenApp    : menu and user interface
Input         : safe input reading helper

CanteenException (extends std::runtime_error)
 ├── ItemNotFoundException
 ├── DuplicateItemException
 ├── OutOfStockException
 ├── OrderNotFoundException
 ├── InvalidOperationException
 └── InvalidInputException
```

---

## 🛠️ OOP Concepts Implemented

* **Classes & Objects:** `MenuItem`, `Menu`, `Order`, `Canteen`, `CanteenApp`, `Input`.
* **Encapsulation:** private data members (price, stock, order status) changed only through public methods like `reduceStock()` and `markPaid()`.
* **Abstraction:** `MenuItem` is an abstract class with pure virtual functions `category()` and `taxRate()`.
* **Inheritance:** `Beverage`, `Snack`, `Meal` and `Dessert` derive from `MenuItem`; custom exceptions derive from `CanteenException`.
* **Polymorphism:** the order calls `item->taxRate()` and `item->category()` on a `shared_ptr<MenuItem>`, and the correct derived-class version runs at runtime.
* **Constructors & Virtual Destructor:** proper initialization and safe cleanup through base pointers.
* **Static Members:** `MenuItem::nextId` (auto item IDs) and `Order::counter` (order numbers).
* **Operator Overloading:** `operator<<` (friend) prints a complete bill with `cout << order`.
* **Exception Handling:** custom exception hierarchy with `try / catch` in the menu loop and in the order loop.
* **Factory Function:** `createItem()` builds the right derived object for the chosen category.
* **STL & Smart Pointers:** `vector`, `map`, `sort` with lambda, `shared_ptr`.

---

## 🚀 How to Compile and Run

### 1. Prerequisites
* A C++ compiler supporting C++11 or later (`g++` from MinGW / MSYS2 / Linux GCC / Clang).

### 2. Compilation
Open your terminal in the project directory and run:
```bash
g++ -std=c++11 main.cpp -o canteen_system
```

### 3. Execution
* **On Windows:**
  ```bash
  canteen_system.exe
  ```
* **On Linux / macOS:**
  ```bash
  ./canteen_system
  ```

> 💡 **Tip:** Choose option `11` (Load Demo Orders) to instantly create 5 sample orders (4 paid, 1 pending) and try the billing and report modules.

---

## 💻 Sample Program Menu

```text
==================================================
            CANTEEN MANAGEMENT SYSTEM
==================================================
 [Module 1: Menu Management]
   1. Display Menu
   2. Add New Item
   3. Update Price / Restock Item
   4. Remove Item

 [Module 2: Order Management]
   5. Place New Order
   6. View All Orders
   7. Cancel Pending Order

 [Module 3: Billing]
   8. Pay & Generate Bill / Reprint Bill

 [Module 4: Sales Reports]
   9. Sales Report (Best Sellers)
  10. Low Stock Alert

  11. Load Demo Orders
   0. Exit System
==================================================
```

---

## 🧾 Sample Output (Bill)

```text
==================================================
              COLLEGE CANTEEN - BILL
==================================================
Order No   : 5003
Customer   : Rohit
Date/Time  : 03-Nov-2026 06:40
Payment    : Card
Status     : PAID
--------------------------------------------------
Item                  Qty       Price       Amount
--------------------------------------------------
Chole Bhature         3         70.00       210.00
Burger                2         50.00       100.00
Coffee                2         20.00        40.00
Ice Cream             2         30.00        60.00
--------------------------------------------------
Subtotal                                    410.00
Bulk Discount (10%)                         -41.00
GST                                          28.53
--------------------------------------------------
GRAND TOTAL (Rs)                            397.53
==================================================
        Thank you! Enjoy your meal.
```

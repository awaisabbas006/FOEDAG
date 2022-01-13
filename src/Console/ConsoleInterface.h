#pragma once

#include <QObject>

class ConsoleInterface : public QObject {
  Q_OBJECT
 public:
  explicit ConsoleInterface(QObject *parent = nullptr);
  virtual ~ConsoleInterface() = default;

  virtual void run(const QString &command) = 0;
  virtual QString startWith() const = 0;

 signals:
  void done();
  void sendOutput(const QString &);
};

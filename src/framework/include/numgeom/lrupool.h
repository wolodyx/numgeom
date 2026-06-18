#ifndef NUMGEOM_FRAMEWORK_LRUPOOL_H
#define NUMGEOM_FRAMEWORK_LRUPOOL_H

#include <memory>
#include <vector>

/** \class LruPool
\brief Пул счетных ресурсов с LRU-стратегией изъятия.

Используемые термины:
* ресурс (`Resource`) -- объект с полезной нагрузкой;
* потребитель (`Consumer`) -- объект, имеющий потребность в ресурсе;
* пул (`LruPool`) -- координатор, связывающий ресурсы с потребителями.

Пул предоставляет три операции:
1) выделение ресурса обращающемуся потребителю;
2) освобождение ресурса потребителем;
3) информирование, что ресурсом воспользовались.

Пул управляет ограниченным набором счетных ресурсов. Когда все ресурсы заняты,
а требуется новый, то ресурс забирается у неактивного потребителя.

Для адаптации пула под свои задачи, следует:
* добавить производный класс от `LruPool::Resource` и добавить в него поля для
  хранения состояния ресурсов;
* добавить производный класс от `LruPool::Consumer` и реализовать методы:
  +) `bool ConfiscateResource`;
  +) `void SetResource(Resource*)`;
* добавить производный класс от `LruPool` и реализовать методы:
  +) `void Initialize()`;
  +) `void Finalize()`;
  +) `Resource* AllocateResourceObject()`;
  +) `void Initialize(Resource*)`;
  +) `void Finalize(Resource*)`.
*/
class LruPool {
 public:
  typedef std::shared_ptr<LruPool> PoolPtr;

  class Resource;

  class Consumer {
   friend class LruPool;
  public:
    Consumer(PoolPtr pool);
    virtual ~Consumer();
    //! Реализация потребителем действий перед изъятием ресурса. Если метод
    //! возвращает `false`, то потребитель не готов расставаться с ресурсом.
    virtual bool ConfiscateResource() = 0;
    //! Пул выделяет потребителю ресурс после запроса через `LruPool::Allocate`.
    virtual void SetResource(Resource*) = 0;
   protected:
    //! Вызов метода указывает, что потребитель воспользовался ресурсом.
    void Use();
    LruPool* GetPool() const;
   private:
    LruPool* lru_pool_;
  };

  class Resource {
    friend class LruPool;
   public:
    Resource(LruPool*);
    virtual ~Resource();
    const LruPool* GetPool() const;
   private:
    LruPool* pool_;
    Consumer* consumer_ = nullptr; //!< Связанный с ресурсом потребитель.
    uint64_t last_used_frame_ = 0;
  };

 public:
  template<typename LruPoolType, class... _Types>
  static PoolPtr Make(_Types&&... _Args) {
    return PoolPtr(new LruPoolType(_Args...),
                   [](LruPool* p) { LruPool::Destroy(p); });
  }

 public:
  LruPool(size_t num_resources);

  size_t GetResourcesCount() const;
  size_t GetFreeResourcesCount() const;

  /** \brief Запрос на выделение ресурса.
  \details Если есть свободный ресурс, то возвращается он.
  Иначе вытесняется тот, что дольше всех не использовался, предварительно
  вызывается `Consumer::ConfiscateResource` у его владельца.
  */
  bool Allocate(Consumer* consumer);

  //! Отметка, что ресурс был использован.
  void Use(Consumer* consumer);

  //! Добровольное освобождение ресурса потребителем.
  //! \return true если ресурс был найден и освобождён.
  bool Free(Consumer* consumer);

 protected:
  virtual ~LruPool();
  static void Destroy(LruPool*);

 private:
  LruPool(const LruPool&) = delete;
  LruPool& operator=(const LruPool&) = delete;

  virtual void Initialize() {} //!< Подготовка пула к выделению ресурсов.
  virtual void Finalize() {} //! Уничтожение пула.
  virtual Resource* AllocateResourceObject() = 0;
  virtual void Initialize(Resource*) = 0;
  virtual void Finalize(Resource*) = 0;

  Resource* FindByConsumer(Consumer* consumer);
  Resource* FindFree();
  Resource* FindLeastRecentlyUsed();

 private:
  std::vector<Resource*> resources_;
  uint64_t frame_counter_ = 0;
};
#endif  // !NUMGEOM_FRAMEWORK_LRUPOOL_H

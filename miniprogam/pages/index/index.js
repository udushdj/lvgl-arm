const app = getApp()

Page({
  data: {
    currentPatient: null,
    waitingList: [],
    waitingCount: 0,
    nextPatient: '',
    totalCount: 0
  },

  onShow() {
    this.loadData()
  },

  loadData() {
    const that = this
    wx.request({
      url: app.globalData.apiBase + '/patient_queue.json',
      method: 'GET',
      success(res) {
        if (res.statusCode === 200 && Array.isArray(res.data)) {
          const patients = res.data
          app.globalData.patients = patients
          that.setData({
            currentPatient: patients.length > 0 ? patients[0] : null,
            waitingList: patients.slice(1),
            waitingCount: patients.length > 0 ? patients.length - 1 : 0,
            nextPatient: patients.length > 1 ? patients[1].Patient_ID : '',
            totalCount: patients.length
          })
        }
      }
    })
  },

  onCallNext() {
    const patients = app.globalData.patients
    if (patients.length === 0) {
      wx.showToast({ title: '队列为空', icon: 'none' })
      return
    }
    const that = this
    wx.showModal({
      title: '确认叫号',
      content: '叫下一位：' + patients[0].name + '（' + patients[0].Patient_ID + '）？',
      success(res) {
        if (res.confirm) {
          patients.shift()
          app.pushPatients(patients, function(ok) {
            if (ok) {
              wx.showToast({ title: '叫号成功', icon: 'success' })
              that.loadData()
            } else {
              wx.showToast({ title: '同步失败', icon: 'none' })
            }
          })
        }
      }
    })
  },

  goQueue() {
    wx.navigateTo({ url: '/pages/queue/queue' })
  },

  goStats() {
    wx.navigateTo({ url: '/pages/stats/stats' })
  },

  goAdd() {
    wx.navigateTo({ url: '/pages/add/add' })
  }
})

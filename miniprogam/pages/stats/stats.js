const app = getApp()

Page({
  data: {
    stats: {},
    queueSize: 0,
    barData: [],
    events: []
  },

  onShow() {
    this.loadAll()
  },

  loadAll() {
    const that = this
    app.loadStats(function(stats) {
      if (stats) {
        that.setData({
          stats: stats,
          events: stats.recent_events || []
        })
        that.buildBars(stats.hourly_calls || [])
      }
    })
    wx.request({
      url: app.globalData.apiBase + '/patient_queue.json',
      method: 'GET',
      success(res) {
        if (res.statusCode === 200 && Array.isArray(res.data)) {
          that.setData({ queueSize: res.data.length })
        }
      }
    })
  },

  buildBars(hourly) {
    const now = new Date()
    const curHour = now.getHours()
    const startHour = (curHour - 7 + 24) % 24
    let maxVal = 1
    for (let i = 0; i < 8; i++) {
      if (hourly[i] > maxVal) maxVal = hourly[i]
    }
    const barData = []
    for (let i = 0; i < 8; i++) {
      const val = hourly[i] || 0
      const h = maxVal > 0 ? Math.max(8, val / maxVal * 160) : 8
      barData.push({
        count: val,
        hour: (startHour + i) % 24,
        height: h
      })
    }
    this.setData({ barData: barData })
  },

  onRefresh() {
    this.loadAll()
    wx.showToast({ title: '已刷新', icon: 'success' })
  }
})

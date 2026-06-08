App({
  globalData: {
    apiBase: 'http://192.168.171.73:8080',
    patients: [],
    stats: null
  },

  onLaunch() {
    this.loadPatients();
  },

  loadPatients() {
    const that = this;
    wx.request({
      url: that.globalData.apiBase + '/patient_queue.json',
      method: 'GET',
      success(res) {
        if (res.statusCode === 200 && Array.isArray(res.data)) {
          that.globalData.patients = res.data;
        }
      }
    });
  },

  pushPatients(patients, callback) {
    const that = this;
    wx.request({
      url: that.globalData.apiBase + '/cgi-bin/api',
      method: 'POST',
      header: { 'content-type': 'application/json' },
      data: patients,
      success(res) {
        if (res.statusCode === 200) {
          that.globalData.patients = patients;
          if (callback) callback(true);
        } else {
          if (callback) callback(false);
        }
      },
      fail() {
        if (callback) callback(false);
      }
    });
  },

  loadStats(callback) {
    const that = this;
    wx.request({
      url: that.globalData.apiBase + '/stats.json',
      method: 'GET',
      success(res) {
        if (res.statusCode === 200 && res.data) {
          that.globalData.stats = res.data;
          if (callback) callback(res.data);
        } else {
          if (callback) callback(null);
        }
      },
      fail() {
        if (callback) callback(null);
      }
    });
  },

  pushStats(stats, callback) {
    const that = this;
    wx.request({
      url: that.globalData.apiBase + '/cgi-bin/api?target=stats',
      method: 'POST',
      header: { 'content-type': 'application/json' },
      data: stats,
      success(res) {
        if (res.statusCode === 200) {
          if (callback) callback(true);
        } else {
          if (callback) callback(false);
        }
      },
      fail() {
        if (callback) callback(false);
      }
    });
  }
})

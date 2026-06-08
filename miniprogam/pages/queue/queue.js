const app = getApp()

Page({
  data: {
    patients: [],
    filteredPatients: [],
    keyword: '',
    sortAsc: true,
    sortLabel: '正序'
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
          app.globalData.patients = res.data
          that.setData({
            patients: res.data,
            filteredPatients: res.data
          })
        }
      }
    })
  },

  onSearchInput(e) {
    this.setData({ keyword: e.detail.value })
  },

  onSearch() {
    const keyword = this.data.keyword.trim()
    if (!keyword) {
      this.setData({ filteredPatients: this.data.patients })
      return
    }
    const filtered = this.data.patients.filter(p =>
      p.name.indexOf(keyword) >= 0 || String(p.Patient_ID).indexOf(keyword) >= 0
    )
    this.setData({ filteredPatients: filtered })
  },

  onSync() {
    this.loadData()
    wx.showToast({ title: '已同步', icon: 'success' })
  },

  onSort() {
    const asc = !this.data.sortAsc
    const sorted = asc
      ? this.data.filteredPatients.slice().sort((a, b) => a.Patient_ID - b.Patient_ID)
      : this.data.filteredPatients.slice().sort((a, b) => b.Patient_ID - a.Patient_ID)
    this.setData({
      sortAsc: asc,
      sortLabel: asc ? '正序' : '倒序',
      filteredPatients: sorted
    })
  },

  onPin(e) {
    const idx = e.currentTarget.dataset.index
    const list = this.data.patients.slice()
    const [moved] = list.splice(idx, 1)
    list.unshift(moved)
    const that = this
    app.pushPatients(list, function(ok) {
      if (ok) {
        wx.showToast({ title: '已置顶', icon: 'success' })
        that.setData({ patients: list, filteredPatients: list })
      }
    })
  },

  onDelete(e) {
    const idx = e.currentTarget.dataset.index
    const list = this.data.patients.slice()
    const name = list[idx].name
    const that = this
    wx.showModal({
      title: '确认删除',
      content: '删除患者「' + name + '」？',
      success(res) {
        if (res.confirm) {
          list.splice(idx, 1)
          app.pushPatients(list, function(ok) {
            if (ok) {
              wx.showToast({ title: '已删除', icon: 'success' })
              that.setData({ patients: list, filteredPatients: list })
            }
          })
        }
      }
    })
  },

  goAdd() {
    wx.navigateTo({ url: '/pages/add/add' })
  }
})

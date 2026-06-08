const app = getApp()

Page({
  data: {
    formData: {
      patientId: '',
      name: '',
      idCard: '',
      age: ''
    },
    errors: {}
  },

  handleInput(e) {
    const field = e.currentTarget.dataset.field
    const value = e.detail.value
    this.setData({
      ['formData.' + field]: value,
      ['errors.' + field]: ''
    })
  },

  handleAgeMinus() {
    const age = parseInt(this.data.formData.age) || 0
    if (age > 0) {
      this.setData({
        'formData.age': String(age - 1),
        'errors.age': ''
      })
    }
  },

  handleAgePlus() {
    const age = parseInt(this.data.formData.age) || 0
    if (age < 120) {
      this.setData({
        'formData.age': String(age + 1),
        'errors.age': ''
      })
    }
  },

  handleCancel() {
    wx.navigateBack()
  },

  handleSave() {
    const { patientId, name, idCard, age } = this.data.formData
    if (!patientId || !name || !age) {
      wx.showToast({ title: '请填写必填项', icon: 'none' })
      return
    }

    const patients = app.globalData.patients || []
    const exists = patients.some(p => p.Patient_ID === patientId)
    if (exists) {
      this.setData({ 'errors.patientId': '该编号已存在' })
      wx.showToast({ title: '患者编号已存在', icon: 'none' })
      return
    }

    patients.push({
      Patient_ID: patientId,
      name: name,
      ID_number: idCard,
      age: age
    })

    const that = this
    app.pushPatients(patients, function(ok) {
      if (ok) {
        wx.showToast({ title: '添加成功', icon: 'success' })
        setTimeout(function() {
          wx.navigateBack()
        }, 500)
      } else {
        wx.showToast({ title: '添加失败', icon: 'none' })
      }
    })
  }
})
